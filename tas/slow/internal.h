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

#ifndef INTERNAL_H_SLOW
#define INTERNAL_H_SLOW

/** @addtogroup tas-sp
 *  @brief TAS Slow Path
 *  @ingroup tas */

#include <stdint.h>

#include <utils_nbqueue.h>
#include <utils_timeout.h>

#include <tas_memif.h>

struct config_route;
struct connection;
struct kernel_statistics;
struct listener;
struct timeout;
enum timeout_type;

extern struct timeout_manager timeout_mgr;
extern struct kernel_statistics kstats;
extern uint32_t cur_ts;
extern int kernel_notifyfd;

struct nicif_completion {
  struct nbqueue_el el;
  struct nbqueue *q;
  int notify_fd;
  int32_t status;
  void *ptr;
};

struct kernel_statistics {
  /** drops detected by flextcp on NIC */
  uint64_t drops;
  /** kernel re-transmission timeouts */
  uint64_t kernel_rexmit;
  /** # of ECN marked ACKs */
  uint64_t ecn_marked;
  /** total number of ACKs */
  uint64_t acks;
  /* 50 percentile rx batch size */
  uint64_t rx_50b;
  /* 75 percentile rx batch size */
  uint64_t rx_75b;
  /* 75 percentile rx batch size */
  uint64_t rx_90b;
  /* 50 percentile tx batch size */
  uint64_t tx_50b;
  /* 75 percentile tx batch size */
  uint64_t tx_75b;
  /* 90 percentile tx batch size */
  uint64_t tx_90b;
};

struct budget_statistics {
  int64_t budget;
  uint64_t cycles_poll;
  uint64_t cycles_tx;
  uint64_t cycles_rx;
  uint64_t cycles_total;
};

/** Type of timeout */
enum timeout_type {
  /** ARP request */
  TO_ARP_REQ,
  /** TCP handshake sent */
  TO_TCP_HANDSHAKE,
  /** TCP retransmission timeout */
  TO_TCP_RETRANSMIT,
  /** TCP connection closed, ready to free */
  TO_TCP_CLOSED,
};

/*****************************************************************************/
/**
 * @addtogroup tas-sp-nicif
 * @brief NIC Interface
 * @ingroup tas-sp
 * @{ */

/** Initialize NIC interface */
int nicif_init(void);

/** Poll NIC queues */
unsigned nicif_poll(void);
/** Poll OvS queue. Only polls when GRE is on */
unsigned ovs_poll(void);
/** Makes an ovs upcall for a received packet */
int ovs_rx_upcall(volatile struct flextcp_pl_krx *krx);
/** Makes an ovs upcall for a packet to be sent */
int ovs_tx_upcall(struct pkt_gre *p, uint16_t vmid,
    uint16_t len, struct connection *conn);

/**
 * Register application context (must be called from poll thread).
 *
 * @param appid    Application ID
 * @param db       Doorbell ID
 * @param rxq_base Base addresses of context receive queue
 * @param rxq_len  Length of context receive queue
 * @param txq_base Base addresses of context transmit queue
 * @param txq_len  Length of context transmit queue
 * @param evfd     Event FD used to ping app
 *
 * @return 0 on success, <0 else
 */
int nicif_appctx_add(uint16_t vmid, uint16_t appid, uint32_t db,
    uint64_t *rxq_base, uint32_t rxq_len, uint64_t *txq_base,
    uint32_t txq_len, int evfd);

/** Flags for connections (used in nicif_connection_add()) */
enum nicif_connection_flags {
  /** Enable ECN for connection. */
  NICIF_CONN_ECN        = (1 <<  2),
};

/**
 * Register flow (must be called from poll thread).
 *
 * @param db          Doorbell ID
 * @param app_id      Application ID
 * @param mac_remote  MAC address of the remote host
 * @param ip_local    Local IP address
 * @param port_local  Local port number
 * @param ip_remote   Remote IP address
 * @param port_remote Remote port number
 * @param rx_base     Base address of circular receive buffer
 * @param rx_len      Length of circular receive buffer
 * @param tx_base     Base address of circular transmit buffer
 * @param tx_len      Length of circular transmit buffer
 * @param remote_seq  Next sequence number expected from remote host
 * @param local_seq   Next sequence number for transmission
 * @param app_opaque  Opaque value to pass in notificaitions
 * @param flags       See #nicif_connection_flags.
 * @param rate        Congestion rate to set [Kbps]
 * @param fn_core     FlexNIC emulator core for the connection
 * @param flow_group  Flow group
 * @param pf_id       Pointer to location where flow id should be stored
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_add(uint32_t db, uint16_t vm_id, uint16_t app_id,
    uint64_t mac_remote, uint32_t ip_local, uint16_t port_local,
    uint32_t ip_remote, uint16_t port_remote, uint64_t rx_base, uint32_t rx_len,
    uint64_t tx_base, uint32_t tx_len, uint32_t remote_seq, uint32_t local_seq,
    uint64_t app_opaque, uint32_t flags, uint32_t rate, uint32_t fn_core,
    uint16_t flow_group, uint32_t *pf_id);

/**
 * Register flow (must be called from poll thread).
 *
 * @param db            Doorbell ID
 * @param app_id        Application ID
 * @param mac_remote    MAC address of the remote host
 * @param out_ip_local  Local IP header of tunnel endpoint
 * @param out_ip_remote Remote IP header of tunnel endpoint
 * @param in_ip_local   Local IP address of VM
 * @param port_local    Local port number
 * @param in_ip_remote  Remote IP address of VM
 * @param port_remote   Remote port number
 * @param rx_base       Base address of circular receive buffer
 * @param rx_len        Length of circular receive buffer
 * @param tx_base       Base address of circular transmit buffer
 * @param tx_len        Length of circular transmit buffer
 * @param remote_seq    Next sequence number expected from remote host
 * @param local_seq     Next sequence number for transmission
 * @param app_opaque    Opaque value to pass in notificaitions
 * @param flags         See #nicif_connection_flags.
 * @param rate          Congestion rate to set [Kbps]
 * @param fn_core       FlexNIC emulator core for the connection
 * @param flow_group    Flow group
 * @param pf_id         Pointer to location where flow id should be stored
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_add_gre(uint32_t db, uint16_t vm_id, uint16_t app_id,
    uint32_t tunnel_id, uint64_t mac_remote,
    uint32_t out_ip_local, uint32_t out_ip_remote,
    uint32_t in_ip_local, uint16_t port_local,
    uint32_t in_ip_remote, uint16_t port_remote, uint64_t rx_base, uint32_t rx_len,
    uint64_t tx_base, uint32_t tx_len, uint32_t remote_seq, uint32_t local_seq,
    uint64_t app_opaque, uint32_t flags, uint32_t rate, uint32_t fn_core,
    uint16_t flow_group, uint32_t *pf_id);

/**
 * Disable connection fast path (mark as sp'd and remove from hash table).
 *
 * @param f_id      Flow state ID
 * @param tx_seq    Pointer to return last transmit sequence number
 * @param rx_seq    Pointer to return last receive sequence number
 * @param tx_closed Pointer to return flag that tx stream is closed
 * @param rx_closed Pointer to return flag that rx stream is closed
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_disable(uint32_t f_id, uint32_t *tx_seq, uint32_t *rx_seq,
    int *tx_closed, int *rx_closed);

/**
 * Free flow state.
 *
 * @param f_id Flow state ID
 */
void nicif_connection_free(uint32_t f_id);

/**
 * Move flow to new db.
 *
 * @param dst_db  New doorbell ID
 * @param f_id    ID of flow to be moved
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_move(uint32_t dst_db, uint32_t f_id);

/**
 * Connection statistics for congestion control
 * (see nicif_connection_stats()).
 */
struct nicif_connection_stats {
  /** Number of dropped segments */
  uint16_t c_drops;
  /** Number of ACKs received */
  uint16_t c_acks;
  /** Acknowledged bytes */
  uint32_t c_ackb;
  /** Number of ACKd bytes with ECN marks */
  uint32_t c_ecnb;
  /** Has pending data in transmit buffer */
  int txp;
  /** Current rtt estimate */
  uint32_t rtt;
  /** Sequence number of next segment to be sent */
  uint32_t c_tx_next_seq;
  /** Number of bytes available to be sent */
  uint32_t c_tx_avail;
};

/**
 * Read connection stats from NIC.
 *
 * @param f_id    ID of flow
 * @param p_stats Pointer to statistics structs.
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_stats(uint32_t f_id,
    struct nicif_connection_stats *p_stats);

/**
 * Set rate for flow.
 *
 * @param f_id  ID of flow
 * @param rate  Rate to set [Kbps]
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_setrate(uint32_t f_id, uint32_t rate);

/**
 * Trigger tx a sigle segment because the packet that
 * signals that the rx window is open probably got dropepd.
 *
 * @param f_id ID of flow
 * @param vm_id ID of vm for flow
 * @param flow_group FlexNIC flow group
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_winretransmit(uint32_t f_id, uint32_t vm_id, uint16_t flow_group);

/**
 * Mark flow for retransmit after timeout.
 *
 * @param f_id ID of flow
 * @param vm_id ID of vm for flow
 * @param flow_group FlexNIC flow group
 *
 * @return 0 on success, <0 else
 */
int nicif_connection_retransmit(uint32_t f_id, uint32_t vm_id, uint16_t core);

/**
 * Allocate transmit buffer for raw packet.
 *
 * TODO: we probably want an asynchronous version of this.
 *
 * @param len     Length of packet to be sent
 * @param buf     Pointer to location where base address will be stored
 * @param opaque  Pointer to location to store opaque value that needs to be
 *                passed to nicif_tx_send().
 *
 * @return 0 on success, <0 else
 */
int nicif_tx_alloc(uint16_t len, void **buf, uint32_t *opaque);

/**
 * Allocate buffer for packet destined to OvSt.
 *
 * @param len     Length of packet to be sent
 * @param buf     Pointer to location where base address will be stored
 * @param opaque  Pointer to location to store opaque value that needs to be
 *                passed to nicif_tx_send().
 *
 * @return 0 on success, <0 else
 */
int nicif_tasovs_tx_alloc(uint16_t len, void **pbuf, uint32_t *opaque);

/**
 * Actually send out transmit buffer (lens need to match).
 *
 * @param opaque Opaque value returned from nicif_tx_alloc().
 * @param no_ts  If != 0, skip inserting tcp timestamp
 *
 * @return 0 on success, <0 else
 */
void nicif_tx_send(uint32_t opaque, int no_ts);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-packetmem
 * @brief Packet Memory Manager.
 * @ingroup tas-sp
 *
 * Manages memory region that can be used by FlexNIC for DMA.
 * @{ */

struct packetmem_handle;

/** Initialize packet memory interface */
int packetmem_init(void);

/**
 * Allocate packet memory of specified length.
 *
 * @param length  Required number of bytes
 * @param off     Pointer to location where offset in DMA region should be
 *                stored
 * @param handle  Pointer to location where handle for memory region should be
 *                stored
 * @param vmid    Id of the vm for the memory region to be allocated
 *
 * @return 0 on success, <0 else
 */
int packetmem_alloc(size_t length, uintptr_t *off,
    struct packetmem_handle **handle, int vmid);

/**
 * Free packet memory region.
 *
 * @param handle  Handle for memory region to be freed
 * @param vmid    Id of the vm that identified mem region to be freed
 *
 * @return 0 on success, <0 else
 */
void packetmem_free(struct packetmem_handle *handle, int vmid);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-appif
 * @brief Application Interface.
 * @ingroup tas-sp
 *
 * This is implemented in appif.c and appif_ctx.c
 * @{ */

/** Initialize application interface */
int appif_init(void);

/** Poll application in memory queues */
unsigned appif_poll(void);

/**
 * Callback from tcp_open(): Connection open done.
 *
 * @param c       Connection
 * @param status  Status: 0 if successful
 */
void appif_conn_opened(struct connection *c, int status);

/**
 * Callback from tcp_close(): Connection close done.
 *
 * @param c       Connection
 * @param status  Status: 0 if successful
 */
void appif_conn_closed(struct connection *c, int status);

/**
 * Callback from TCP module: New connection request received on listener.
 *
 * @param l           Listener that received new connection
 * @param remote_ip   Remote IP address
 * @param remote_port Remote port
 */
void appif_listen_newconn(struct listener *l, uint32_t remote_ip,
    uint16_t remote_port);

/**
 * Callback from TCP module: New connection request with GRE tunnel
 * received on listener.
 *
 * @param l              Listener that received new connection
 * @param out_remote_ip  Remote IP address of tunnel endpoint
 * @param in_remote_ip   Remote IP address of VM
 * @param remote_port    Remote port
 * @param tunnel_id      ID of the tunnel
 */
void appif_listen_newconn_gre(struct listener *l,
    uint32_t out_remote_ip, uint32_t in_remote_ip,
    uint16_t remote_port, uint32_t tunnel_id);

/**
 * Callback from tcp_accept(): Connection accepted.
 *
 * @param c       Connection passed to tcp_accept
 * @param status  Status: 0 if successful
 */
void appif_accept_conn(struct connection *c, int status);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-tcp
 * @brief TCP Protocol Handling
 * @ingroup tas-sp
 * @{ */

/** TCP connection state machine state. */
enum connection_status {
  /** Accepted: waiting for a SYN. */
  CONN_SYN_WAIT,
  /** Opening: wairing for OvS response. */
  CONN_OVS_PENDING,
  /** Opening: received OvS response. */
  CONN_OVS_COMP,
  /** Opening: waiting for ARP request. */
  CONN_ARP_PENDING,
  /** Opening: SYN request sent. */
  CONN_SYN_SENT,
  /** Opening: SYN received, waiting for NIC registration. */
  CONN_REG_SYNACK,
  /** Connection opened. */
  CONN_OPEN,
  /** Connection closed. */
  CONN_CLOSED,
  /** Connection failed. */
  CONN_FAILED,
};

/** Congestion control data for window-based DCTCP */
struct connection_cc_dctcp_win {
  /** Rate of ECN bits received. */
  uint32_t ecn_rate;
  /** Congestion window. */
  uint32_t window;
  /** Flag indicating whether flow is in slow start. */
  int slowstart;
};

/** Congestion control data for window-based DCTCP */
struct connection_cc_dctcp_rate {
  /** Unprocessed acks */
  uint32_t unproc_acks;
  /** Unprocessed ack bytes */
  uint32_t unproc_ackb;
  /** Unprocessed ECN ack bytes */
  uint32_t unproc_ecnb;
  /** Unprocessed drops */
  uint32_t unproc_drops;

  /** Rate of ECN bits received. */
  uint32_t ecn_rate;
  /** Actual rate. */
  uint32_t act_rate;
  /** Flag indicating whether flow is in slow start. */
  int slowstart;
};

/** Congestion control data for TIMELY */
struct connection_cc_timely {
  /** Previous RTT. */
  uint32_t rtt_prev;
  /** RTT gradient. */
  int32_t rtt_diff;
  /** HAI counter. */
  uint32_t hai_cnt;
  /** Actual rate. */
  uint32_t act_rate;
  /** Last timestamp. */
  uint32_t last_ts;
  /** Flag indicating whether flow is in slow start. */
  int slowstart;
};

/** TCP connection state */
struct connection {
  /**
   * @name Application interface
   * @{
   */
    /** Application-specified opaque value for connection. */
    uint64_t opaque;
    /** Application context this connection is assigned to. */
    struct app_context *ctx;
    /** New application context if connection should be moved. */
    struct app_context *new_ctx;
    /** Link list pointer for application connections. */
    struct connection *app_next;
    /** Doorbell id. */
    uint32_t db_id;
  /**@}*/

  /**
   * @name Data buffers
   * @{
   */
    /** Memory manager handle for receive buffer. */
    struct packetmem_handle *rx_handle;
    /** Memory manager handle for transmit buffer. */
    struct packetmem_handle *tx_handle;
    /** Receive buffer pointer. */
    uint8_t *rx_buf;
    /** Transmit buffer pointer. */
    uint8_t *tx_buf;
    /** Receive buffer size. */
    uint32_t rx_len;
    /** Transmit buffer size. */
    uint32_t tx_len;
  /**@}*/

  /**
   * @name Address information
   * @{
   */
    /** Peer MAC address for connection. */
    uint64_t remote_mac;
    /** GRE tunnel ID */
    uint32_t tunnel_id;
    /** Peer IP address of outer header. */
    uint32_t out_remote_ip;
    /** Local IP to be used in outer header. */
    uint32_t out_local_ip;
    /** Peer IP address of inner header. */
    uint32_t in_remote_ip;
    /** Local IP to be used in inner header. */
    uint32_t in_local_ip;
    /** Peer port number. */
    uint16_t remote_port;
    /** Local port number. */
    uint16_t local_port;
  /**@}*/

  /**
   * @name Connection state
   * @{
   */
    /** Current connection state machine state. */
    enum connection_status status;
    /** Peer sequence number. */
    uint32_t remote_seq;
    /** Local sequence number. */
    uint32_t local_seq;
    /** Timestamp received with SYN/SYN-ACK packet */
    uint32_t syn_ts;
  /**@}*/

  /**
   * @name Timeouts
   * @{
   */
    /** Timeout in microseconds (used for handshake). */
    uint32_t timeout;
    /** Timeout object. */
    struct timeout to;
    /** Number of times timout triggered. */
    int to_attempts;
    /** 1 if timeout is currently armed. */
    int to_armed;
  /**@}*/

  /**
   * @name Congestion control
   * @{
   */
    /** Timestamp when control loop ran last */
    uint32_t cc_last_ts;
    /** Last rtt estimate */
    uint32_t cc_rtt;
    /** Number of dropped segments */
    uint16_t cc_last_drops;
    /** Number of ACKs received */
    uint16_t cc_last_acks;
    /** Acknowledged bytes */
    uint32_t cc_last_ackb;
    /** Number of ACKd bytes with ECN marks */
    uint32_t cc_last_ecnb;
    /** Sequence number last time control loop ran */
    uint32_t cc_last_tx_next_seq;

    /** Congestion rate limit. */
    uint32_t cc_rate;
    /** Had retransmits. */
    uint32_t cc_rexmits;
    /** Data for CC algorithm. */
    union {
      /** Window-based dctcp */
      struct connection_cc_dctcp_win dctcp_win;
      /** TIMELY */
      struct connection_cc_timely timely;
      /** Rate-based dctcp */
      struct connection_cc_dctcp_rate dctcp_rate;
    } cc;
    /** control intervals without acking window update */
    uint32_t cnt_win_updt_pending;
    /** Timestamp when window update first got stuck */
    uint32_t ts_win_updt_pending;
    /** control intervals with data in tx buffer but no ACKs */
    uint32_t cnt_tx_pending;
    /** Timestamp when flow was first not moving */
    uint32_t ts_tx_pending;
    /** Linked list for CC connection list. */
    struct connection *cc_next;
  /**@}*/

  /** Linked list in hash table. */
  struct connection *ht_next;
  /** Asynchronous completion information. */
  struct nicif_completion comp;
  /** NIC flow state ID. */
  uint32_t flow_id;
  /** FlexNIC emulator core. */
  uint32_t fn_core;
  /** Flags: see #nicif_connection_flags */
  uint32_t flags;
  /** Flow group (RSS bucket for steering). */
  uint16_t flow_group;
};

/** App context element in listener list. When a
 *  a process gets forked it gets added to the
 *  listener list of forked contexts.
 */
 struct forked_context {
  /** Application context for this fork */
  struct app_context *ctx;
  /** Link list pointer for forked contexts */
  struct forked_context *next;
};

/** TCP listener  */
struct listener {
  /**
   * @name Application interface
   * @{
   */
    /** Application-specified opaque value for listener. */
    uint64_t opaque;
    /** Application context this listener is assigned to. */
    struct app_context *ctx;
    /** Link list pointer for application listeners. */
    struct listener *app_next;
    /** Doorbell id. */
    uint32_t db_id;
  /**@}*/

  /**
   * @name Backlog queue
   * @{
   */
    /** Backlog queue total length. */
    uint32_t backlog_len;
    /** Next entry in backlog queue. */
    uint32_t backlog_pos;
    /** Number of entries used in backlog queue. */
    uint32_t backlog_used;
    /** Backlog queue buffers */
    void **backlog_ptrs;
    /** Backlog core id array */
    uint32_t *backlog_cores;
    /** Backlog flow group array */
    uint16_t *backlog_fgs;
  /**@}*/

  /** List of waiting connections from accept calls (head) */
  struct connection *wait_conns;
  /** List of waiting connections from accept calls (tail) */
  struct connection *wait_conns_last;

  /** Listener port */
  uint16_t port;
  /** Flags: see #nicif_connection_flags */
  uint32_t flags;
};

/** List of tcp connections */
extern struct connection *tcp_conns;

/** Initialize TCP subsystem */
int tcp_init(void);

/** Poll for TCP events */
void tcp_poll(void);

/**
 * Open a connection.
 *
 * This function returns asynchronously if it does not fail immediately. The TCP
 * module will call appif_conn_opened().
 *
 * @param ctx             Application context
 * @param opaque          Opaque value passed from application
 * @param remote_ip    Remote IP address of VM
 * @param remote_port     Remote port number
 * @param db_id           Doorbell ID to use for connection
 * @param conn            Pointer to location for storing pointer of created conn
 *                        struct.
 *
 * @return 0 on success, <0 else
 */
int tcp_open(struct app_context *ctx,
    uint64_t opaque, uint32_t remote_ip,
    uint16_t remote_port, uint32_t db_id, struct connection **conn);

/**
 * Open a listener.
 *
 * @param ctx         Application context
 * @param opaque      Opaque value passed from application
 * @param local_port  Port to listen on
 * @param backlog     Backlog queue length
 * @param reuseport   Enable reuseport, to have multiple listeners for the same
 *                    port.
 * @param listen      Pointer to location for storing pointer of created
 *                    listener struct.
 *
 * @return 0 on success, <0 else
 */
int tcp_listen(struct app_context *ctx, uint64_t opaque, uint16_t local_port,
    uint32_t backlog, int reuseport, struct listener **listen);

/**
 * Prepare to receive a connection on a listener.
 *
 * @param ctx     Application context
 * @param opaque  Opaque value passed from application
 * @param listen  Listener
 * @param db_id   Doorbell ID
 *
 * @return 0 on success, <0 else
 */
int tcp_accept(struct app_context *ctx, uint64_t opaque,
        struct listener *listen, uint32_t db_id);

/**
 * RX processing for a TCP packet.
 *
 * @param pkt Pointer to packet
 * @param len Length of packet
 * @param fn_core FlexNIC emulator core
 * @param flow_group Flow group (rss bucket for steering)
 *
 * @return 0 if packet has been consumed, <0 otherwise.
 */
int tcp_packet(const void *pkt, uint16_t len, uint32_t fn_core,
    uint16_t flow_group);

/**
 * RX processing for an encapsulated TCP packet.
 *
 * @param pkt Pointer to packet
 * @param len Length of packet
 * @param fn_core FlexNIC emulator core
 * @param flow_group Flow group (rss bucket for steering)
 *
 * @return 0 if packet has been consumed, <0 otherwise.
 */
int gre_packet(const void *pkt, uint16_t len, uint32_t fn_core,
    uint16_t flow_group);

/**
 * Destroy already closed/failed connection.
 *
 * @param conn  Connection
 */
int tcp_close(struct connection *conn);

/**
 * Destroy already closed/failed connection.
 *
 * @param conn  Connection
 */
void tcp_destroy(struct connection *conn);

/**
 * TCP timeout triggered.
 *
 * @param to    Timeout that triggered
 * @param type  Timeout type
 */
void tcp_timeout(struct timeout *to, enum timeout_type type);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-cc
 * @brief Congestion Control
 * @ingroup tas-sp
 * @{ */

/** Initialize congestion control management */
int cc_init(void);

/**
 * Poll congestion control
 *
 * @param cur_ts Current timestamp in micro seconds.
 */
unsigned cc_poll(uint32_t cur_ts);

uint32_t cc_next_ts(uint32_t cur_ts);

/**
 * Initialize congestion state for flow
 *
 * @param conn Connection to initialize.
 */
void cc_conn_init(struct connection *conn);

/**
 * Remove congestion state for flow
 *
 * @param conn Connection to remove.
 */
void cc_conn_remove(struct connection *conn);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-arp
 * @brief ARP Protocol Handling
 * @ingroup tas-sp
 * @{ */

/** Initialize ARP subsystem */
int arp_init(void);

/**
 * Resolve IP address to MAC address using ARP resolution.
 *
 * This function can either return success immediately in case on an ARP cache
 * hit, or return asynchronously if an ARP request was sent out.
 *
 * @param comp  Context for asynchronous return
 * @param ip    IP address to be resolved
 * @param mac   Pointer of memory location where destination MAC should be
 *              stored.
 *
 * @return 0 on success, < 0 on error, and > 0 if request was sent but response
 *    is still pending.
 */
int arp_request(struct nicif_completion *comp, uint32_t ip, uint64_t *mac);

/**
 * RX processing for an ARP packet.
 *
 * @param pkt Pointer to packet
 * @param len Length of packet
 */
void arp_packet(const void *pkt, uint16_t len);

/**
 * ARP timeout triggered.
 *
 * @param to    Timeout that triggered
 * @param type  Timeout type
 */
void arp_timeout(struct timeout *to, enum timeout_type type);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-routing
 * @brief IP routing
 * @ingroup tas-sp
 * @{ */

/** Initialize IP routing subsystem */
int routing_init(void);

/**
 * Resolve IP address to MAC address using routing and ARP.
 *
 * This function can either return success immediately, or asynchronously.
 *
 * @param comp  Context for asynchronous return
 * @param ip    IP address to be resolved
 * @param mac   Pointer of memory location where destination MAC should be
 *              stored.
 *
 * @return 0 on success, < 0 on error, and > 0 for asynchronous return.
 */
int routing_resolve(struct nicif_completion *comp, uint32_t ip,
    uint64_t *mac);

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-kni
 * @brief Host Kernel Interface.
 * @ingroup tas-sp
 *
 * This is implemented in kni.c
 * @{ */


/** Initialize kni if enabled */
int kni_init(void);

/** Pass packet to KNI if enabled (buffer is not consumed). */
void kni_packet(const void *pkt, uint16_t len);

/** Poll kni */
unsigned kni_poll(void);

/** @} */

#endif // ndef INTERNAL_H_SLOW

/** @} */

/*****************************************************************************/
/**
 * @addtogroup tas-sp-appif_connect
 * @brief App Interface Connect
 * @ingroup tas-sp
 *
 * This is implemented in appif_connect.c
 * @{ */


/* Accept connection to app interface */
int appif_connect_accept(int cfd, int cores_num,
    int kernel_notifyfd, int shm_fd);

/*****************************************************************************/
/**
 * @addtogroup tas-sp-tas
 * @brief General stuff in tas.c
 * @ingroup tas-sp
 *
 * This is implemented in tas.c
 * @{ */


/* Get reamining budget for a VM in specified core */
uint64_t tas_get_budget(int vmid, int ctxid);