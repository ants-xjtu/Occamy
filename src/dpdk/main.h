/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <unistd.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_hash.h>
#include <rte_interrupts.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_spinlock.h>
#include <rte_tcp.h>

#define MAX_NAME_LEN 100

#ifndef APP_MBUF_ARRAY_SIZE
#define APP_MBUF_ARRAY_SIZE 256
#endif

#define FORWARD_ENTRY 15 // num of forwarding table entries

#define ECN_THRESHOLD 99840
#define HEADDROP_FREQ 5e7
#define SPEED_10G_LIMIT 95e8

#define SWITCH_MAX_FREQ 55e6
#define HEADDROP_BUCKET_MAX_SIZE 200

#define BUCKET_MAX_SIZE 5000

#define ENABLE_ECN 1

// 0 -> DRR   1 -> PQ
#ifdef SCHED_DRR
#define SCHD 0
#elif SCHED_PQ
#define SCHD 1
#endif
#define LOW_PRIO_SHIFT_ALPHA 0
#define HIGH_PRIO_SHIFT_ALPHA 3

#define APP_CORE_NUM 16

struct app_mbuf_array {
  struct rte_mbuf *array[APP_MBUF_ARRAY_SIZE];
  uint16_t n_mbufs;
};

struct token_bucket {
  long long token;
  uint64_t pre_tsc;
};

#ifndef APP_MAX_PORTS
#define APP_MAX_PORTS 8
#endif

#ifndef APP_MAX_QUEUES
#define APP_MAX_QUEUES 9
#endif

typedef void (*buffer_info_func_t)(uint32_t port_id, uint32_t queue_id,
                                   uint32_t pkt_len, bool add, bool remain);

struct port_state {
  uint64_t num_of_pkts;
  uint64_t num_of_bit;
  uint64_t start_tsc;
};

struct queue_abminfo {
  uint64_t num_of_byte;
};

struct port_abminfo {
  uint64_t num_of_byte;
  uint64_t start_tsc;
  uint64_t queue_abm_info[APP_MAX_QUEUES];
  // double aqm[APP_MAX_QUEUES];
};

struct app_params {
  /* CPU cores */
  uint32_t core_rx;
  uint32_t core_worker;
  uint32_t core_tx[APP_MAX_PORTS];
  uint32_t core_debug;
  uint32_t core_headdrop;
  uint32_t core_func;
  uint32_t core_threshold;
  uint32_t core_update_aqm;
  uint32_t core_free;

  /* Ports*/
  uint32_t ports[APP_MAX_PORTS];
  uint32_t n_ports;
  uint32_t n_queues;

  /* token bucket */
  struct token_bucket bucket[APP_MAX_PORTS];

  /* Rings */
  struct rte_ring *rings_rx[APP_MAX_PORTS];
  struct rte_ring *rings_tx[APP_MAX_PORTS][APP_MAX_QUEUES];
  struct rte_ring *rings_to_free;
  uint32_t ring_rx_size;
  uint32_t ring_tx_size;
  uint32_t ring_free_size;

  /* Internal buffers */
  struct app_mbuf_array mbuf_rx[APP_MAX_PORTS];
  struct app_mbuf_array mbuf_wk[APP_MAX_PORTS];
  struct app_mbuf_array mbuf_tx[APP_MAX_PORTS][APP_MAX_QUEUES];
  struct app_mbuf_array mbuf_tx_test[APP_MAX_PORTS];
  struct app_mbuf_array mbuf_free;

  /* Buffer pool */
  struct rte_mempool *pool;
  uint32_t pool_buffer_size;
  uint32_t pool_size;
  uint32_t pool_cache_size;

  /* Burst sizes */
  uint32_t burst_size_rx_read;
  uint32_t burst_size_rx_write;
  uint32_t burst_size_worker_read;
  uint32_t burst_size_worker_write;
  uint32_t burst_size_tx_read;
  uint32_t burst_size_tx_write;

  /* Ethernet addresses */
  struct rte_ether_addr port_eth_addr[APP_MAX_PORTS];
  struct rte_ether_addr next_hop_eth_addr[APP_MAX_PORTS];

  /* memory method */
  int method;

  /* schedule method */
  int schedule_method;

  /* threshold for abm */
  double T[APP_MAX_PORTS][APP_MAX_QUEUES];
  int threshold_abm[APP_MAX_PORTS][APP_MAX_QUEUES];

  /* buffer info */
  uint32_t threshold;
  int dt_shift_alpha;
  uint32_t pri_shift_alpha;
  uint64_t buffer_size;
  uint64_t reserve;
  uint64_t total_rsrv;
  uint32_t total_remain;
  uint64_t qlen[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t occu_size_in[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t occu_size_out[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t occu_size_drop[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t shared_used_in;
  uint64_t shared_used_out;
  uint64_t shared_used_drop;
  uint64_t shared_used_port_in[APP_MAX_PORTS];
  uint64_t shared_used_port_out[APP_MAX_PORTS];
  uint64_t shared_used_port_drop[APP_MAX_PORTS];
  uint64_t shared_used_queue_in[APP_MAX_QUEUES];
  uint64_t shared_used_queue_out[APP_MAX_QUEUES];
  uint64_t shared_used_queue_drop[APP_MAX_QUEUES];

  uint64_t shared_used_each_out[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t shared_used_each_in[APP_MAX_PORTS][APP_MAX_QUEUES];

  uint64_t tot_occu_in;

  uint64_t tot_occu_out;
  uint64_t tot_occu_port_in[APP_MAX_PORTS];
  uint64_t tot_occu_port_out[APP_MAX_PORTS];
  uint64_t tot_occu_port_drop[APP_MAX_PORTS];
  uint32_t rx_port[APP_MAX_PORTS];

  /* DRR info */
  int deflict_counter[APP_MAX_PORTS][APP_MAX_QUEUES];
  int quantum[APP_MAX_QUEUES];

  /* out port state */
  struct port_state port_state[APP_MAX_PORTS];
  struct port_abminfo port_abm_info[APP_MAX_PORTS];
  struct port_abminfo abm_info[APP_MAX_PORTS];

  /* Debug */
  FILE *fp;
  FILE *in_file;
  FILE *out_file;

  /* things about forwarding table */
  uint32_t fwd_table[FORWARD_ENTRY]; // 转发表   hash -> port_id
  struct rte_hash *l2_hash;          // hash函数

  char mac_file[MAX_NAME_LEN];

  /* debug info */
  uint64_t drop[APP_MAX_PORTS][APP_MAX_QUEUES];
  uint64_t scc[APP_MAX_PORTS][APP_MAX_QUEUES];

  /* output freq */
  uint64_t output_token_out[APP_MAX_PORTS];
} __rte_cache_aligned;

extern struct app_params app;

/* parse basic args of switch */
int app_parse_args(int argc, char **argv);
/* print usage of this switch */
void app_print_usage(void);
/* init this switch */
void app_init(void);
/* each lcore will do this loop */
int app_lcore_main_loop(void *arg);
/* rx | get the packet from NIC and bring it to the ingress queue */
void app_main_loop_rx(void);
/* worker | bring packet from ingress queue to egress queue */
void app_main_loop_worker(int (*packet_to_egress)(struct rte_mbuf *, uint32_t));
void app_main_loop_tx(uint32_t port_id,
                      void (*update_info)(uint32_t ii, uint32_t jj,
                                          uint32_t pkt_len1, bool b1, bool b2));
void app_main_loop_tx_sp(uint32_t port_id,
                         void (*update_info)(uint32_t ii, uint32_t jj,
                                             uint32_t pkt_len1, bool b1,
                                             bool b2));

/* this core is to print queue length dynamicly as debug info */
void app_main_loop_printinfo();
/* this core is updating threshold all the time for ABM*/
void app_main_loop_update_T();
/* this core is doing headdrop for occamy method*/
void app_main_loop_headdrop(void);
/* this core is doing headdrop for occamy method*/
void app_main_loop_headdrop_longest(void);
/* this core is calculating the DT/occamy threshold all the time */
void app_main_loop_threshold(void);
/* this core is updating aqm as a given frequency for ABM*/
void app_main_loop_update_aqm(void);
/* get a pkt's priority */
uint32_t get_priority(struct rte_mbuf *m);
/* check if this packet can enter the queue for DT */
int check_admit(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len);
/* check if this packet can enter the queue for pushout */
int check_admit_pushout(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len);
/* check if this packet can enter the queue for abm */
int check_admit_abm(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len);
/* check if this packet can enter the queue for occamy */
int check_admit_plus(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len);
/* the buffer info will update each time a pkt enter the egress or leave the
 * egress, this func is for DT and occamy */
void update_buffer_info(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len,
                        bool add, bool remain);
/* the buffer info will update each time a pkt enter the egress or leave the
 * egress, this func is for pushout */
void update_buffer_info_pushout(uint32_t port_id, uint32_t queue_id,
                                uint32_t pkt_len, bool add, bool pushout);
/* the buffer info will update each time a pkt enter the egress or leave the
 * egress, this func is for ABM */
void update_buffer_info_abm(uint32_t port_id, uint32_t queue_id,
                            uint32_t pkt_len, bool add, bool remain);

/* bring packet to egress for DT */
int packet_to_egress(struct rte_mbuf *m, uint32_t port_id);
/* bring packet to egress for pushout */
int packet_to_egress_pushout(struct rte_mbuf *m, uint32_t port_id);
/* bring packet to egress for ABM */
int packet_to_egress_abm(struct rte_mbuf *m, uint32_t port_id);
/* bring packet to egress for occamy */
int packet_to_egress_occamy(struct rte_mbuf *m, uint32_t port_id);
/* bring packet to egress for ST(complete partition) */
int packet_to_egress_cp(struct rte_mbuf *m, uint32_t port_id);

/* get the occupy size of one queue in a port */
uint64_t get_occu_size(uint32_t port_id, uint32_t queue_id);
/* get the buffer size of this switch */
uint64_t get_buffer_size();
/* get the total occupied size */
void update_aqm(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len,
                uint64_t sec_tsc);
#endif /* _MAIN_H_ */
