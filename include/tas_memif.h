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

#ifndef FLEXTCP_PLIF_H_
#define FLEXTCP_PLIF_H_

#include <stdint.h>
#include <utils.h>
#include <stdatomic.h>
#include <packet_defs.h>

/**
 * @addtogroup tas-fp
 * @brief TAS Fast Path
 * @ingroup tas
 * @{ */

#define BATCH_SIZE 16
#define BATCH_STATS

#define FLEXNIC_HUGE_PREFIX "/dev/hugepages"

/** Name for the info shared memory region. */
#define FLEXNIC_NAME_INFO "tas_info"
/** Name for flexnic dma shared memory region. */
#define FLEXNIC_NAME_DMA_MEM "tas_memory"
/** Name for flexnic internal shared memory region. */
#define FLEXNIC_NAME_INTERNAL_MEM "tas_internal"

/** Size of the info shared memory region. */
#define FLEXNIC_INFO_BYTES 0x1000

/** Indicates that flexnic is done initializing. */
#define FLEXNIC_FLAG_READY 1
/** Indicates that huge pages should be used for the internal and dma memory */
#define FLEXNIC_FLAG_HUGEPAGES 2

/** ID of the mem region to use for the slow path */
#define SP_MEM_ID FLEXNIC_PL_VMST_NUM

/** NIC buffer struc used by kernel queues */
struct nic_buffer
{
  uint64_t addr;
  void *buf;
};

/** Info struct: layout of info shared memory region */
struct flexnic_info {
  /** Flags: see FLEXNIC_FLAG_* */
  uint64_t flags;
  /** Size of flexnic dma memory in bytes. */
  uint64_t dma_mem_size;
  /** Offset of flexnic dma memory in bytes */
  uint64_t dma_mem_off;
  /** Size of internal flexnic memory in bytes. */
  uint64_t internal_mem_size;
  /** Size of NIC rx queue */
  uint32_t nic_rx_len;
  /** Size of NIC tx queue */
  uint32_t nic_tx_len;
  /** export mac address */
  uint64_t mac_address;
  /** Cycles to poll before blocking for application */
  uint64_t poll_cycle_app;
  /** Cycles to poll before blocking for TAS */
  uint64_t poll_cycle_tas;
  /** Number of queues in queue manager */
  uint32_t qmq_num;
  /** Number of cores in flexnic emulator */
  uint32_t cores_num;
} __attribute__((packed));



/******************************************************************************/
/* Kernel RX queue */

#define FLEXTCP_PL_KRX_INVALID 0x0
#define FLEXTCP_PL_KRX_PACKET 0x1

/** Kernel RX queue entry. */
struct flextcp_pl_krx {
  uint64_t addr;
  union {
    struct {
      uint16_t len;
      uint16_t fn_core;
      uint16_t flow_group;
      uint16_t vmid;
    } packet;
    uint8_t raw[55];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_krx) == 64, krx_size);


/******************************************************************************/
/* Kernel TX queue */

#define FLEXTCP_PL_KTX_INVALID 0x0
#define FLEXTCP_PL_KTX_PACKET 0x1
#define FLEXTCP_PL_KTX_CONNRETRAN 0x2
#define FLEXTCP_PL_KTX_WINRETRAN 0x3
#define FLEXTCP_PL_KTX_PACKET_NOTS 0x4

/** Kernel TX queue entry */
struct flextcp_pl_ktx {
  union {
    struct {
      uint64_t addr;
      uint16_t len;
    } packet;
    struct {
      uint32_t flow_id;
    } connretran;
    uint8_t raw[63];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_ktx) == 64, ktx_size);

/******************************************************************************/
/* TAS to OvS Entry*/

#define FLEXTCP_PL_TOE_INVALID 0x0
#define FLEXTCP_PL_TOE_VALID 0x1

/** TAS to OvS queue entry. */
struct flextcp_pl_toe {
  uint64_t addr;
  union {
    struct {
      uint16_t len;
      uint16_t fn_core;
      uint16_t flow_group;
      uint16_t vmid;
      uint64_t connaddr;
    } packet;
    uint8_t raw[55];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_toe) == 64, toe_size);


/******************************************************************************/
/* OvS to TAS Entry */

#define FLEXTCP_PL_OTE_INVALID 0x0
#define FLEXTCP_PL_OTE_VALID 0x1

/** OvS to TAS queue entry */
struct flextcp_pl_ote {
  uint64_t addr;
  uint32_t key;
  uint32_t out_remote_ip;
  uint32_t out_local_ip;
  uint32_t in_remote_ip;
  uint32_t in_local_ip;
  union {
    struct {
      uint16_t len;
      uint16_t fn_core;
      uint16_t flow_group;
      uint16_t vmid;
      uint64_t connaddr;
    } packet;
    uint8_t raw[35];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_ote) == 64, ote_size);

/******************************************************************************/

/******************************************************************************/
/* App RX queue */

#define FLEXTCP_PL_ARX_INVALID    0x0
#define FLEXTCP_PL_ARX_CONNUPDATE 0x1

#define FLEXTCP_PL_ARX_FLRXDONE  0x1

/** Update receive and transmit buffer of flow */
struct flextcp_pl_arx_connupdate {
  uint64_t opaque;
  uint32_t rx_bump;
  uint32_t rx_pos;
  uint32_t tx_bump;
  uint8_t flags;
} __attribute__((packed));

/** Application RX queue entry */
struct flextcp_pl_arx {
  union {
    struct flextcp_pl_arx_connupdate connupdate;
    uint8_t raw[31];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_arx) == 32, arx_size);

/******************************************************************************/
/* App TX queue */

#define FLEXTCP_PL_ATX_CONNUPDATE 0x1

#define FLEXTCP_PL_ATX_FLTXDONE  0x1

/** Application TX queue entry */
struct flextcp_pl_atx {
  union {
    struct {
      uint32_t rx_bump;
      uint32_t tx_bump;
      uint32_t flow_id;
      uint16_t bump_seq;
      uint8_t  flags;
    } __attribute__((packed)) connupdate;
    uint8_t raw[15];
  } __attribute__((packed)) msg;
  volatile uint8_t type;
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct flextcp_pl_atx) == 16, atx_size);

/******************************************************************************/
/* Internal flexnic memory */

#define FLEXNIC_PL_VMST_NUM         6
#define FLEXNIC_PL_APPST_NUM        8
#define FLEXNIC_PL_APPST_CTX_NUM   31
#define FLEXNIC_PL_APPST_CTX_MCS   16
#define FLEXNIC_PL_APPCTX_NUM      16
#define FLEXNIC_PL_FLOWST_NUM     (128 * 1024)
#define FLEXNIC_PL_FLOWHT_ENTRIES (FLEXNIC_PL_FLOWST_NUM * 2)
#define FLEXNIC_PL_FLOWHT_NBSZ      4

/** Application state */
struct flextcp_pl_appst {
  /********************************************************/
  /* read-only fields */

  /** Number of contexts */
  uint16_t ctx_num;

  /** IDs of contexts */
  uint16_t ctx_ids[FLEXNIC_PL_APPST_CTX_NUM];
} __attribute__((packed));


/** Application context registers */
struct flextcp_pl_appctx {
  /********************************************************/
  /* read-only fields */
  uint64_t rx_base;
  uint64_t tx_base;
  uint32_t rx_len;
  uint32_t tx_len;
  uint32_t vm_id;
  int	   evfd;

  /********************************************************/
  /* read-write fields */
  uint64_t last_ts;
  uint32_t rx_head;
  uint32_t tx_head;
  uint32_t rx_avail;
} __attribute__((packed));

/** Enable out of order receive processing members */
#define FLEXNIC_PL_OOO_RECV 1

#define FLEXNIC_PL_FLOWST_SLOWPATH 1
#define FLEXNIC_PL_FLOWST_ECN 8
#define FLEXNIC_PL_FLOWST_TXFIN 16
#define FLEXNIC_PL_FLOWST_RXFIN 32
#define FLEXNIC_PL_FLOWST_RX_MASK (~63ULL)

/** Tunnel entry */
struct flextcp_pl_tun {
  /** Tunnel ID */
  uint32_t tun_id;
  /** IP of local VM */
  uint32_t in_local_ip;
  /** IP of remote VM */
  uint32_t in_remote_ip;
  /** IP in local end of the tunnel */
  uint32_t out_local_ip;
  /** IP in remote enf of the tunnel */
  uint32_t out_remote_ip;
} __attribute__((packed));


/** Flow state registers */
struct flextcp_pl_flowst {
  /********************************************************/
  /* read-only fields */

  /** Opaque flow identifier from application */
  uint64_t opaque;

  /** Base address of receive buffer */
  uint64_t rx_base_sp;
  /** Base address of transmit buffer */
  uint64_t tx_base;

  /** Length of receive buffer */
  uint32_t rx_len;
  /** Length of transmit buffer */
  uint32_t tx_len;

  /* GRE ID used to identify tunnel */
  beui32_t tunnel_id;
  /* If packet is not GRE encapsulated use only
     out ips */
  beui32_t out_local_ip;
  beui32_t out_remote_ip;
  beui32_t in_local_ip;
  beui32_t in_remote_ip;

  beui16_t local_port;
  beui16_t remote_port;

  /** Remote MAC address */
  struct tas_eth_addr remote_mac;

  /** Doorbell ID (identifying the app ctx to use) */
  uint16_t db_id;

  /** Id of applicatiion this flow belongs to */
  uint16_t app_id;

  /** Id of VM this flow belongs to */
  uint16_t vm_id;

  /** Flow group for this connection (rss bucket) */
  uint16_t flow_group;
  /** Sequence number of queue pointer bumps */
  uint16_t bump_seq;

  // 56

  /********************************************************/
  /* read-write fields */

  /** spin lock */
  volatile uint32_t lock;

  /** Bytes available for received segments at next position */
  uint32_t rx_avail;
  // 64
  /** Offset in buffer to place next segment */
  uint32_t rx_next_pos;
  /** Next sequence number expected */
  uint32_t rx_next_seq;
  /** Bytes available in remote end for received segments */
  uint32_t rx_remote_avail;
  /** Duplicate ack count */
  uint32_t rx_dupack_cnt;

#ifdef FLEXNIC_PL_OOO_RECV
  /* Start of interval of out-of-order received data */
  uint32_t rx_ooo_start;
  /* Length of interval of out-of-order received data */
  uint32_t rx_ooo_len;
#endif

  /** Number of bytes available to be sent */
  uint32_t tx_avail;
  /** Number of bytes up to next pos in the buffer that were sent but not
   * acknowledged yet. */
  uint32_t tx_sent;
  /** Offset in buffer for next segment to be sent */
  uint32_t tx_next_pos;
  /** Sequence number of next segment to be sent */
  uint32_t tx_next_seq;
  /** Timestamp to echo in next packet */
  uint32_t tx_next_ts;

  /** Congestion control rate [kbps] */
  uint32_t tx_rate;
  /** Counter drops */
  uint16_t cnt_tx_drops;
  /** Counter acks */
  uint16_t cnt_rx_acks;
  /** Counter bytes sent */
  uint32_t cnt_rx_ack_bytes;
  /** Counter acks marked */
  uint32_t cnt_rx_ecn_bytes;
  /** RTT estimate */
  uint32_t rtt_est;

// 128
} __attribute__((packed, aligned(64)));

#define FLEXNIC_PL_FLOWHTE_VALID  (1 << 31)
#define FLEXNIC_PL_FLOWHTE_POSSHIFT 29

/** Flow lookup table entry */
struct flextcp_pl_flowhte {
  uint32_t flow_id;
  uint32_t flow_hash;
} __attribute__((packed));

#define FLEXNIC_PL_MAX_FLOWGROUPS 4096

/** OvS state */
struct flextcp_pl_ovsctx {
  /********************************************************/
  /* read-only fields */
  uint64_t rx_base;
  uint32_t rx_len;
  uint64_t tx_base;
  uint32_t tx_len;

  /********************************************************/
  /* read-write fields */
  uint32_t rx_head;
  uint32_t rx_tail;
  uint32_t tx_head;
  uint32_t tx_tail;
} __attribute__((packed));

/** Layout of internal pipeline memory */
struct flextcp_pl_mem {
  /* registers for application context queues */
  struct flextcp_pl_appctx appctx[FLEXNIC_PL_APPST_CTX_MCS]
      [FLEXNIC_PL_VMST_NUM][FLEXNIC_PL_APPCTX_NUM];

  /* registers for flow state */
  struct flextcp_pl_flowst flowst[FLEXNIC_PL_FLOWST_NUM];

  /* flow lookup table */
  struct flextcp_pl_flowhte flowht[FLEXNIC_PL_FLOWHT_ENTRIES];

  /* registers for kernel queues */
  struct flextcp_pl_appctx kctx[FLEXNIC_PL_APPST_CTX_MCS];

  /* registers for application state */
  struct flextcp_pl_appst appst[FLEXNIC_PL_APPST_NUM];

  /* register for tas to ovs queue */
  struct flextcp_pl_ovsctx tasovs;

  /* register for ovs to tas queue */
  struct flextcp_pl_ovsctx ovstas;

  /* histogram for rx batch sizes */
  uint64_t rx_batch_hist[BATCH_SIZE + 1];

  /* histogram for tx batch sizes */
  uint64_t tx_batch_hist[BATCH_SIZE + 1];

  uint8_t flow_group_steering[FLEXNIC_PL_MAX_FLOWGROUPS];
} __attribute__((packed));

/** @} */

#endif /* ndef FLEXTCP_PLIF_H_ */
