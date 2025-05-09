/*
 * Copyright 2019 University of Washington, Max Planck Institute for
 * Software Systems, and The University of Texas at Austin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <rte_config.h>
#include <rte_malloc.h>
#include <rte_cycles.h>

#include <tas_memif.h>
#include <virtuoso.h>

#include "internal.h"
#include "fastemu.h"

#define DATAPLANE_TSCS

#ifdef DATAPLANE_STATS
#ifdef DATAPLANE_TSCS
#define STATS_TS(n) uint64_t n = rte_get_tsc_cycles()
#define STATS_TSADD(c, f, n) __sync_fetch_and_add(&c->stat_##f, n)
#else
#define STATS_TS(n) \
  do                \
  {                 \
  } while (0)
#define STATS_TSADD(c, f, n) \
  do                         \
  {                          \
  } while (0)
#endif
#define STATS_ADD(c, f, n) __sync_fetch_and_add(&c->stat_##f, n)
#else
#define STATS_TS(n) \
  do                \
  {                 \
  } while (0)
#define STATS_TSADD(c, f, n) \
  do                         \
  {                          \
  } while (0)
#define STATS_ADD(c, f, n) \
  do                       \
  {                        \
  } while (0)
#endif

static void dataplane_block(struct dataplane_context *ctx, uint32_t ts);
static unsigned poll_rx(struct dataplane_context *ctx, uint32_t ts,
                        uint64_t tsc) __attribute__((noinline));
static unsigned poll_queues(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
static unsigned poll_active_queues(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
static unsigned poll_all_queues(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
;
static unsigned poll_kernel(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
static unsigned poll_qman(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
static unsigned poll_qman_fwd(struct dataplane_context *ctx, uint32_t ts) __attribute__((noinline));
static void poll_scale(struct dataplane_context *ctx);

static void polled_vm_init(struct polled_vm *app, uint16_t id);
static void polled_ctx_init(struct polled_context *ctx, uint32_t id, uint32_t a_id);

static inline void bufcache_free(struct dataplane_context *ctx,
                                 struct network_buf_handle *handle);

static inline void tx_flush(struct dataplane_context *ctx);
static inline void tx_send(struct dataplane_context *ctx,
                           struct network_buf_handle *nbh, uint16_t off, uint16_t len);

static void spend_budget(struct dataplane_context *ctx, 
    uint64_t cycles);

static void arx_cache_flush(struct dataplane_context *ctx, uint64_t tsc) __attribute__((noinline));

int dataplane_init(void)
{
  if (FLEXNIC_INTERNAL_MEM_SIZE < sizeof(struct flextcp_pl_mem))
  {
    fprintf(stderr, "dataplane_init: internal flexnic memory size not "
                    "sufficient (got %x, need %zx)\n",
            FLEXNIC_INTERNAL_MEM_SIZE,
            sizeof(struct flextcp_pl_mem));
    return -1;
  }

  if (fp_cores_max > FLEXNIC_PL_APPST_CTX_MCS)
  {
    fprintf(stderr, "dataplane_init: more cores than FLEXNIC_PL_APPST_CTX_MCS "
                    "(%u)\n",
            FLEXNIC_PL_APPST_CTX_MCS);
    return -1;
  }
  if (FLEXNIC_PL_FLOWST_NUM > FLEXNIC_NUM_QMQUEUES)
  {
    fprintf(stderr, "dataplane_init: more flow states than queue manager queues"
                    "(%u > %u)\n",
            FLEXNIC_PL_FLOWST_NUM, FLEXNIC_NUM_QMQUEUES);
    return -1;
  }

  return 0;
}

int dataplane_context_init(struct dataplane_context *ctx)
{
  int i, j;
  char name[32];
  struct polled_vm *p_vm;
  struct polled_context *p_ctx;

  /* initialize forwarding queue */
  sprintf(name, "qman_fwd_ring_%u", ctx->id);
  if ((ctx->qman_fwd_ring = rte_ring_create(name, 32 * 1024, rte_socket_id(),
                                            RING_F_SC_DEQ)) == NULL)
  {
    fprintf(stderr, "initializing rte_ring_create\n");
    return -1;
  }

  /* initialize queue manager */
  if (tas_qman_thread_init(ctx) != 0) {
    fprintf(stderr, "initializing qman thread failed\n");
    return -1;
  }

  /* initialize network queue */
  if (network_thread_init(ctx) != 0)
  {
    fprintf(stderr, "initializing rx thread failed\n");
    return -1;
  }

  for (i = 0; i < FLEXNIC_PL_VMST_NUM; i++)
  {
    /* Initialize budget for each VM */
    ctx->budgets[i].vmid = i;
    ctx->budgets[i].budget = config.bu_max_budget;

    /* Set phase counters to 0 */
    ctx->vm_counters[i] = 0;
    
    /* Initialized polled apps and polled vms*/
    p_vm = &ctx->polled_vms[i];
    polled_vm_init(p_vm, i);
    for (j = 0; j < FLEXNIC_PL_APPCTX_NUM; j++)
    {
      p_ctx = &p_vm->ctxs[j];
      polled_ctx_init(p_ctx, j, i);
    }
  }
  
  ctx->counters_total = 0;
  ctx->poll_rounds = 0;
  ctx->poll_next_vm = 0;
  ctx->act_head = IDXLIST_INVAL;
  ctx->act_tail = IDXLIST_INVAL;

  ctx->evfd = eventfd(0, EFD_NONBLOCK);
  assert(ctx->evfd != -1);
  ctx->ev.epdata.event = EPOLLIN;
  int r = rte_epoll_ctl(RTE_EPOLL_PER_THREAD, EPOLL_CTL_ADD, ctx->evfd, &ctx->ev);
  assert(r == 0);

  fp_state->kctx[ctx->id].evfd = ctx->evfd;

  return 0;
}

static void polled_vm_init(struct polled_vm *vm, uint16_t id)
{
  vm->id = id;
  vm->next = IDXLIST_INVAL;
  vm->prev = IDXLIST_INVAL;
  vm->flags = 0;
  vm->poll_next_ctx = 0;
  vm->act_ctx_head = IDXLIST_INVAL;
  vm->act_ctx_tail = IDXLIST_INVAL;
}

static void polled_ctx_init(struct polled_context *ctx, uint32_t id, uint32_t aid)
{
  ctx->id = id;
  ctx->vmid = aid;
  ctx->next = IDXLIST_INVAL;
  ctx->prev = IDXLIST_INVAL;
  ctx->flags = 0;
  ctx->null_rounds = 0;
}

void dataplane_context_destroy(struct dataplane_context *ctx)
{
}

void dataplane_loop(struct dataplane_context *ctx)
{
  struct notify_blockstate nbs;
  uint32_t ts;
  uint64_t cyc, prev_cyc, s_cycs, e_cycs;
  int was_idle = 1;

  notify_canblock_reset(&nbs);
  while (!exited)
  {
    unsigned n = 0;

    /* count cycles of previous iteration if it was busy */
    prev_cyc = cyc;
    cyc = rte_get_tsc_cycles();
    if (!was_idle)
      ctx->loadmon_cyc_busy += cyc - prev_cyc;

    ts = tas_qman_timestamp(cyc);

    STATS_TS(start);
  
    s_cycs = util_rdtsc();
    n += poll_rx(ctx, ts, cyc);
    e_cycs = util_rdtsc();
    spend_budget(ctx, e_cycs - s_cycs);

    STATS_TS(rx);
    tx_flush(ctx);
    
    n += poll_qman_fwd(ctx, ts);

    STATS_TSADD(ctx, cyc_rx, rx - start);
   
    s_cycs = util_rdtsc();
    n += poll_qman(ctx, ts);
    e_cycs = util_rdtsc();
    spend_budget(ctx, e_cycs - s_cycs);
   
    STATS_TS(qm);
    STATS_TSADD(ctx, cyc_qm, qm - rx);
   
    s_cycs = util_rdtsc();
    n += poll_queues(ctx, ts);
    e_cycs = util_rdtsc();
    spend_budget(ctx, e_cycs - s_cycs);
  
    STATS_TS(qs);
    STATS_TSADD(ctx, cyc_qs, qs - qm);
  
    s_cycs = util_rdtsc();
    n += poll_kernel(ctx, ts);
    e_cycs = util_rdtsc();
    spend_budget(ctx, e_cycs - s_cycs);

    /* flush transmit buffer */
    tx_flush(ctx);

    if (ctx->id == 0)
      poll_scale(ctx);

    was_idle = (n == 0);
    if (config.fp_interrupts && notify_canblock(&nbs, !was_idle, cyc))
    {
      dataplane_block(ctx, ts);
      notify_canblock_reset(&nbs);
    }
  }
}

static void dataplane_block(struct dataplane_context *ctx, uint32_t ts)
{
  uint32_t max_timeout;
  uint64_t val;
  int ret, i;
  struct rte_epoll_event event[2];

  if (network_rx_interrupt_ctl(&ctx->net, 1) != 0)
  {
    return;
  }

  max_timeout = tas_qman_next_ts(&ctx->qman, ts);

  ret = rte_epoll_wait(RTE_EPOLL_PER_THREAD, event, 2,
                       max_timeout == (uint32_t)-1 ? -1 : max_timeout / 1000);
  if (ret < 0)
  {
    perror("dataplane_block: rte_epoll_wait failed");
    abort();
  }

  for (i = 0; i < ret; i++)
  {
    if (event[i].fd == ctx->evfd)
    {
      ret = read(ctx->evfd, &val, sizeof(uint64_t));
      if ((ret > 0 && ret != sizeof(uint64_t)) ||
          (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
      {
        perror("dataplane_block: read failed");
        abort();
      }
    }
  }
  network_rx_interrupt_ctl(&ctx->net, 0);
}

#ifdef DATAPLANE_STATS
static inline uint64_t read_stat(uint64_t *p)
{
  return __sync_lock_test_and_set(p, 0);
}

void dataplane_dump_stats(void)
{
  struct dataplane_context *ctx;
  unsigned i;

  for (i = 0; i < fp_cores_max; i++)
  {
    ctx = ctxs[i];
    fprintf(stderr, "dp stats %u: "
                    "qm=(%" PRIu64 ",%" PRIu64 ",%" PRIu64 ")  "
                    "rx=(%" PRIu64 ",%" PRIu64 ",%" PRIu64 ")  "
                    "qs=(%" PRIu64 ",%" PRIu64 ",%" PRIu64 ")  "
                    "cyc=(%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ")\n",
            i,
            read_stat(&ctx->stat_qm_poll), read_stat(&ctx->stat_qm_empty),
            read_stat(&ctx->stat_qm_total),
            read_stat(&ctx->stat_rx_poll), read_stat(&ctx->stat_rx_empty),
            read_stat(&ctx->stat_rx_total),
            read_stat(&ctx->stat_qs_poll), read_stat(&ctx->stat_qs_empty),
            read_stat(&ctx->stat_qs_total),
            read_stat(&ctx->stat_cyc_db), read_stat(&ctx->stat_cyc_qm),
            read_stat(&ctx->stat_cyc_rx), read_stat(&ctx->stat_cyc_qs));
  }
}
#endif

static unsigned poll_rx(struct dataplane_context *ctx, uint32_t ts,
                        uint64_t tsc)
{
  int ret;
  unsigned i, n;
  uint8_t freebuf[BATCH_SIZE] = {0};
  void *fss[BATCH_SIZE];
  struct tcp_opts tcpopts[BATCH_SIZE];
  struct network_buf_handle *bhs[BATCH_SIZE];

  n = BATCH_SIZE;
  if (TXBUF_SIZE - ctx->tx_num < n)
    n = TXBUF_SIZE - ctx->tx_num;

  STATS_ADD(ctx, rx_poll, 1);

  /* receive packets */
  ret = network_poll(&ctx->net, n, bhs);
  if (ret <= 0)
  {
    STATS_ADD(ctx, rx_empty, 1);
    return 0;
  }

  STATS_ADD(ctx, rx_total, n);
  n = ret;

  /* prefetch packet contents (1st cache line) */
  for (i = 0; i < n; i++)
  {
    rte_prefetch0(network_buf_bufoff(bhs[i]));
  }

  /* look up flow states */
  #if VIRTUOSO_GRE
    fast_flows_packet_fss_gre(ctx, bhs, fss, n);
  #else
    fast_flows_packet_fss(ctx, bhs, fss, n);
  #endif

  /* prefetch packet contents (2nd cache line, TS opt overlaps) */
  for (i = 0; i < n; i++)
  {
    rte_prefetch0(network_buf_bufoff(bhs[i]) + 64);
  }

  /* parse packets */
  #if VIRTUOSO_GRE
    fast_flows_packet_parse_gre(ctx, bhs, fss, tcpopts, n);
  #else
    fast_flows_packet_parse(ctx, bhs, fss, tcpopts, n);
  #endif

  for (i = 0; i < n; i++)
  {
    /* run fast-path for flows with flow state */
    if (fss[i] != NULL)
    {
      #if VIRTUOSO_GRE
        ret = fast_flows_packet_gre(ctx, bhs[i], fss[i], &tcpopts[i], ts);
      #else
        ret = fast_flows_packet(ctx, bhs[i], fss[i], &tcpopts[i], ts);
      #endif
    }
    else
    {
      ret = -1;
    }

    if (ret > 0)
    {
      freebuf[i] = 1;
    }
    else if (ret < 0)
    {
      fast_kernel_packet(ctx, bhs[i], fss[i]);
    }
  }

  arx_cache_flush(ctx, tsc);

  /* free received buffers */
  for (i = 0; i < n; i++)
  {
    if (freebuf[i] == 0)
      bufcache_free(ctx, bhs[i]);
  }

  return n;
}

static unsigned poll_queues(struct dataplane_context *ctx, uint32_t ts)
{
  unsigned total;

  if (ctx->poll_rounds % MAX_POLL_ROUNDS == 0 || ctx->act_head == IDXLIST_INVAL)
  {
    total = poll_all_queues(ctx, ts);
  }
  else
  {
    total = poll_active_queues(ctx, ts);
  }
  ctx->poll_rounds = (ctx->poll_rounds + 1) % MAX_POLL_ROUNDS;

  return total;
}

/* Polls active applications that have recently sent data */
static unsigned poll_active_queues(struct dataplane_context *ctx, uint32_t ts)
{
  int ret, n_rem = 0;
  struct network_buf_handle **handles;
  void *aqes[BATCH_SIZE];
  struct polled_context *rem_ctxs[BATCH_SIZE];
  unsigned total = 0;
  uint16_t max, i, k = 0, num_bufs = 0;

  STATS_ADD(ctx, qs_poll, 1);

  max = BATCH_SIZE;
  if (TXBUF_SIZE - ctx->tx_num < max)
    max = TXBUF_SIZE - ctx->tx_num;

  /* allocate buffers contents */
  max = bufcache_prealloc(ctx, max, &handles);

  /* prefetch all active contexts */
  fast_appctx_poll_pf_active(ctx);

  /* fetch packets from active contexts */
  k = fast_appctx_poll_fetch_active(ctx, max, &total, &n_rem, rem_ctxs, aqes);

  for (i = 0; i < k; i++)
  {
    ret = fast_appctx_poll_bump(ctx, aqes[i], handles[num_bufs], ts);
    if (ret == 0)
      num_bufs++;
  }

  /* apply buffer reservations */
  bufcache_alloc(ctx, num_bufs);

  /* probe receive queue on all active contexts */
  fast_actx_rxq_probe_active(ctx);

  /* update round */
  ctx->act_head = ctx->polled_vms[ctx->act_head].next;
  ctx->act_tail = ctx->polled_vms[ctx->act_tail].next;

  /* remove contexts and apps that have not sent for a few rounds
     from active list */
  remove_ctxs_from_active(ctx, rem_ctxs, n_rem);

  STATS_ADD(ctx, qs_total, total);
  if (total == 0)
    STATS_ADD(ctx, qs_empty, total);

  return total;
}

/* Polls the queues for every application and context */
static unsigned poll_all_queues(struct dataplane_context *ctx, uint32_t ts)
{
  int ret;
  struct network_buf_handle **handles;
  void *aqes[BATCH_SIZE];
  unsigned total = 0;
  uint16_t max, k, i, num_bufs = 0;

  STATS_ADD(ctx, qs_poll, 1);

  max = BATCH_SIZE;
  if (TXBUF_SIZE - ctx->tx_num < max)
    max = TXBUF_SIZE - ctx->tx_num;

  /* allocate buffers contents */
  max = bufcache_prealloc(ctx, max, &handles);

  /* prefetch every ctx from every app */
  fast_appctx_poll_pf_all(ctx);

  /* fetch up to max pkts from groups in round robin fashion */
  k = fast_appctx_poll_fetch_all(ctx, max, &total, aqes);

  for (i = 0; i < k; i++)
  {
    ret = fast_appctx_poll_bump(ctx, aqes[i], handles[num_bufs], ts);
    if (ret == 0)
      num_bufs++;
  }

  /* apply buffer reservations */
  bufcache_alloc(ctx, num_bufs);

  /* probe every ctx from every app */
  fast_actx_rxq_probe_all(ctx);

  STATS_ADD(ctx, qs_total, total);
  if (total == 0)
    STATS_ADD(ctx, qs_empty, total);

  return total;
}

static unsigned poll_kernel(struct dataplane_context *ctx, uint32_t ts)
{
  struct network_buf_handle **handles;
  unsigned total = 0;
  uint16_t max, k = 0;
  int ret;

  max = BATCH_SIZE;
  if (TXBUF_SIZE - ctx->tx_num < max)
    max = TXBUF_SIZE - ctx->tx_num;

  max = (max > 8 ? 8 : max);
  /* allocate buffers contents */
  max = bufcache_prealloc(ctx, max, &handles);

  for (k = 0; k < max;)
  {
    ret = fast_kernel_poll(ctx, handles[k], ts);

    if (ret == 0)
      k++;
    else if (ret < 0)
      break;

    total++;
  }

  /* apply buffer reservations */
  bufcache_alloc(ctx, k);

  return total;
}

static unsigned poll_qman(struct dataplane_context *ctx, uint32_t ts)
{
  unsigned fq_ids[BATCH_SIZE];
  unsigned vq_ids[BATCH_SIZE];
  uint16_t q_bytes[BATCH_SIZE];
  struct network_buf_handle **handles;
  uint16_t off = 0, max;
  int ret, i, use;

  max = BATCH_SIZE;
  if (TXBUF_SIZE - ctx->tx_num < max)
    max = TXBUF_SIZE - ctx->tx_num;

  STATS_ADD(ctx, qm_poll, 1);

  /* allocate buffers contents */
  max = bufcache_prealloc(ctx, max, &handles);

  /* poll queue manager */
  ret = tas_qman_poll(ctx, max, vq_ids, fq_ids, q_bytes);

  if (ret <= 0)
  {
    STATS_ADD(ctx, qm_empty, 1);
    return 0;
  }

  STATS_ADD(ctx, qm_total, ret);
  for (i = 0; i < ret; i++)
  {
    rte_prefetch0(handles[i]);
  }

  for (i = 0; i < ret; i++)
  {
    rte_prefetch0((uint8_t *)handles[i] + 64);
  }

  /* prefetch packet contents */
  for (i = 0; i < ret; i++)
  {
    rte_prefetch0(network_buf_buf(handles[i]));
  }

  fast_flows_qman_pf(ctx, fq_ids, ret);

  fast_flows_qman_pfbufs(ctx, fq_ids, ret);

  for (i = 0; i < ret; i++)
  {
    use = fast_flows_qman(ctx, vq_ids[i], fq_ids[i], handles[off], ts);
    if (use == 0)
      off++;

  }
  
  /* apply buffer reservations */
  bufcache_alloc(ctx, off);

  return ret;
}

static unsigned poll_qman_fwd(struct dataplane_context *ctx, uint32_t ts)
{
  void *flow_states[4 * BATCH_SIZE];
  int ret, i;

  /* poll queue manager forwarding ring */
  ret = rte_ring_dequeue_burst(ctx->qman_fwd_ring, flow_states, 4 * BATCH_SIZE, NULL);
  for (i = 0; i < ret; i++)
  {
    fast_flows_qman_fwd(ctx, flow_states[i]);
  }

  return ret;
}

uint8_t bufcache_prealloc(struct dataplane_context *ctx, uint16_t num,
                          struct network_buf_handle ***handles)
{
  uint16_t grow, res, head, g, i;
  struct network_buf_handle *nbh;

  /* try refilling buffer cache */
  if (ctx->bufcache_num < num)
  {
    grow = BUFCACHE_SIZE - ctx->bufcache_num;
    head = (ctx->bufcache_head + ctx->bufcache_num) & (BUFCACHE_SIZE - 1);

    if (head + grow <= BUFCACHE_SIZE)
    {
      res = network_buf_alloc(&ctx->net, grow, ctx->bufcache_handles + head);
    }
    else
    {
      g = BUFCACHE_SIZE - head;
      res = network_buf_alloc(&ctx->net, g, ctx->bufcache_handles + head);

      if (res == g)
      {
        res += network_buf_alloc(&ctx->net, grow - g, ctx->bufcache_handles);
      }
    }

    for (i = 0; i < res; i++)
    {
      g = (head + i) & (BUFCACHE_SIZE - 1);
      nbh = ctx->bufcache_handles[g];
      ctx->bufcache_handles[g] = (struct network_buf_handle *)((uintptr_t)nbh);
    }

    ctx->bufcache_num += res;
  }
  num = TAS_MIN(num, (ctx->bufcache_head + ctx->bufcache_num <= BUFCACHE_SIZE ? ctx->bufcache_num : BUFCACHE_SIZE - ctx->bufcache_head));

  *handles = ctx->bufcache_handles + ctx->bufcache_head;

  return num;
}

void bufcache_alloc(struct dataplane_context *ctx, uint16_t num)
{
  assert(num <= ctx->bufcache_num);

  ctx->bufcache_head = (ctx->bufcache_head + num) & (BUFCACHE_SIZE - 1);
  ctx->bufcache_num -= num;
}

static inline void bufcache_free(struct dataplane_context *ctx,
                                 struct network_buf_handle *handle)
{
  uint32_t head, num;

  num = ctx->bufcache_num;
  if (num < BUFCACHE_SIZE)
  {
    /* free to cache */
    head = (ctx->bufcache_head + num) & (BUFCACHE_SIZE - 1);
    ctx->bufcache_handles[head] = handle;
    ctx->bufcache_num = num + 1;
    network_buf_reset(handle);
  }
  else
  {
    /* free to network buffer manager */
    network_free(1, &handle);
  }
}

static inline void tx_flush(struct dataplane_context *ctx)
{
  int ret;
  unsigned i;

  if (ctx->tx_num == 0)
  {
    return;
  }

  /* try to send out packets */
  ret = network_send(&ctx->net, ctx->tx_num, ctx->tx_handles);

  if (ret == ctx->tx_num)
  {
    /* everything sent */
    ctx->tx_num = 0;
  }
  else if (ret > 0)
  {
    /* move unsent packets to front */
    for (i = ret; i < ctx->tx_num; i++)
    {
      ctx->tx_handles[i - ret] = ctx->tx_handles[i];
    }
    ctx->tx_num -= ret;
  }
}

static void poll_scale(struct dataplane_context *ctx)
{
  unsigned st = fp_scale_to;

  if (st == 0)
    return;

  fprintf(stderr, "Scaling fast path from %u to %u\n", fp_cores_cur, st);
  if (st < fp_cores_cur)
  {
    if (network_scale_down(fp_cores_cur, st) != 0)
    {
      fprintf(stderr, "network_scale_down failed\n");
      abort();
    }
  }
  else if (st > fp_cores_cur)
  {
    if (network_scale_up(fp_cores_cur, st) != 0)
    {
      fprintf(stderr, "network_scale_up failed\n");
      abort();
    }
  }
  else
  {
    fprintf(stderr, "poll_scale: warning core number didn't change\n");
  }

  fp_cores_cur = st;
  fp_scale_to = 0;
}

static void arx_cache_flush(struct dataplane_context *ctx, uint64_t tsc)
{
  uint16_t i, vmid;
  struct flextcp_pl_appctx *actx;
  struct flextcp_pl_arx *parx[BATCH_SIZE];

  for (i = 0; i < ctx->arx_num; i++)
  {
    vmid = ctx->arx_vm[i];
    actx = &fp_state->appctx[ctx->id][vmid][ctx->arx_ctx[i]];
    if (fast_actx_rxq_alloc(ctx, actx, &parx[i], vmid) != 0)
    {
      /* TODO: how do we handle this? */
      fprintf(stderr, "arx_cache_flush: no space in app rx queue\n");
      abort();
    }
  }

  for (i = 0; i < ctx->arx_num; i++)
  {
    rte_prefetch0(parx[i]);
  }

  for (i = 0; i < ctx->arx_num; i++)
  {
    *parx[i] = ctx->arx_cache[i];
  }

  for (i = 0; i < ctx->arx_num; i++)
  {
    vmid = ctx->arx_vm[i];
    actx = &fp_state->appctx[ctx->id][vmid][ctx->arx_ctx[i]];
    notify_appctx(actx, tsc);
  }

  ctx->arx_num = 0;
}

static void spend_budget(struct dataplane_context *ctx, uint64_t cycles)
{
  int vmid;
  double counter, ratio;
  uint64_t vm_cycles;
  double counters_sum = 0;

  if (ctx->counters_total == 0)
    return;

  for (vmid = 0; vmid < FLEXNIC_PL_VMST_NUM; vmid++)
  {
    counter = ctx->vm_counters[vmid];
    counters_sum += counter;
    ratio = counter / ctx->counters_total;
    assert(counter <= ctx->counters_total);
    assert(ratio >= 0 && ratio <= 1);
    vm_cycles = cycles * ratio;
    __sync_fetch_and_sub(&ctx->budgets[vmid].budget, vm_cycles);
    // ctx->budgets[vmid].budget -= vm_cycles;
    ctx->vm_counters[vmid] = 0;
  }

  assert(counters_sum == ctx->counters_total);
  ctx->counters_total = 0;
}
