/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/time.h>
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
#include <rte_icmp.h>
#include <rte_interrupts.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_tcp.h>

#include "main.h"

#define ENABLE_L2_LEARNING 0
#define QUERY_ECN_MARK 0

int app_l2_lookup(const struct rte_ether_addr *addr) {
  int index = rte_hash_lookup(app.l2_hash, addr);
  if (index >= 0 && index < FORWARD_ENTRY) {
    return app.fwd_table[index];
  }
  return -1;
}

void app_l2_learning(struct rte_ether_addr *addr, uint32_t port_id) {
  if (app_l2_lookup(addr) >= 0)
    return;
  int index = rte_hash_add_key(app.l2_hash, addr);
  printf("learning ... port_id = %u\n", port_id);
  if (index < 0)
    rte_panic("l2_hash add key error \n");
  app.fwd_table[index] = port_id;
}

static int get_dest_port(struct rte_mbuf *m, uint32_t port_id) {
  struct rte_ether_hdr *eth;
  // l2 learning
  eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
#if ENABLE_L2_LEARNING
  app_l2_learning(&(eth->src_addr), port_id);
#endif
  uint8_t *dst_addr;
  dst_addr = eth->dst_addr.addr_bytes;
#if ENABLE_L2_LEARNING
  /* Check if the destination MAC address is the broadcast address */
  if (dst_addr[0] == 0xff && dst_addr[1] == 0xff && dst_addr[2] == 0xff &&
      dst_addr[3] == 0xff && dst_addr[4] == 0xff && dst_addr[5] == 0xff) {
    /* The destination MAC address is a broadcast address */
    return -1;
  }
#endif

  return app_l2_lookup(&(eth->dst_addr));
}

void app_main_loop_rx(void) {
  uint32_t i, j;
  const uint64_t sec_tsc = rte_get_tsc_hz();

  RTE_LOG(INFO, USER1, "Core %u is doing RX\n", rte_lcore_id());

  for (i = 0;; i = ((i + 1) % app.n_ports)) {
    uint16_t n_mbufs, n_batch, n_read, n_write;
    n_mbufs = app.mbuf_rx[i].n_mbufs;
    n_batch = APP_MBUF_ARRAY_SIZE - n_mbufs;
    if (app.burst_size_rx_read < n_batch)
      n_batch = app.burst_size_rx_read;

    /* Read */
    if (n_batch != 0) {
      n_read = rte_eth_rx_burst(app.ports[i], 0, &app.mbuf_rx[i].array[n_mbufs],
                                n_batch);
      n_mbufs += n_read;
    }
    if (n_mbufs == 0)
      continue;
    /* Send to worker */
    n_write = rte_ring_sp_enqueue_burst(
        app.rings_rx[i], (void *)app.mbuf_rx[i].array, n_mbufs, NULL);
    if (0 < n_write && n_write < n_mbufs)
      rte_memcpy((void *)app.mbuf_rx[i].array,
                 (void *)&app.mbuf_rx[i].array[n_write],
                 sizeof(struct rte_mbuf *) * (n_mbufs - n_write));
    app.mbuf_rx[i].n_mbufs = n_mbufs - n_write;
  }
}

void app_main_loop_worker(int (*packet_to_egress)(struct rte_mbuf *,
                                                  uint32_t)) {
  uint32_t i;

  RTE_LOG(INFO, USER1, "Core %u is doing work\n", rte_lcore_id());

  for (i = 0;; i = ((i + 1) % app.n_ports)) {
    uint16_t n_mbufs, n_batch, n_read;
    int ret;
    uint32_t j, k;
    uint32_t queue_id;
    struct rte_mbuf *work_mbuf;
    n_mbufs = app.mbuf_wk[i].n_mbufs;
    n_batch = 16 - n_mbufs;
    if (app.burst_size_worker_read < n_batch)
      n_batch = app.burst_size_worker_read;

    /* Read */
    if (n_batch != 0) {
      n_read = rte_ring_sc_dequeue_burst(
          app.rings_rx[i], (void **)&app.mbuf_wk[i].array[n_mbufs], n_batch,
          NULL);
      n_mbufs += n_read;
    }

    if (n_mbufs == 0)
      continue;
    /* Send to TX */
    ret = 0;
    for (j = 0; j < n_mbufs; j++) {
      if (unlikely(app.mbuf_wk[i].array[j]->pkt_len < 64 ||
                   app.mbuf_wk[i].array[j]->pkt_len > 1518)) {
        rte_pktmbuf_free(app.mbuf_wk[i].array[j]);
        continue;
      }
      int dst_port = get_dest_port(app.mbuf_wk[i].array[j], i);

      // RTE_LOG(INFO, USER1, "from  %d to %d \n", i, dst_port);
      if (unlikely(dst_port == -1)) {
#if ENABLE_L2_LEARNING
        // broadcast
        for (k = 0; k < app.n_ports; k++) {
          if (k == i || k == (i ^ 1))
            continue;
          work_mbuf = rte_pktmbuf_clone(app.mbuf_wk[i].array[j], app.pool);
          ret = packet_to_egress(work_mbuf, k);
        }
#endif
        ret = packet_to_egress((void *)app.mbuf_wk[i].array[j], i ^ 1);
      } else {
        ret = packet_to_egress((void *)app.mbuf_wk[i].array[j], dst_port);
      }
      if (ret != 0) {
        rte_pktmbuf_free(app.mbuf_wk[i].array[j]);
        continue;
      }
    }
  }
}

static int mark_packet_with_ecn(struct rte_mbuf *pkt) {
  if (!ENABLE_ECN || !RTE_ETH_IS_IPV4_HDR(pkt->packet_type))
    return -1;

  struct rte_ipv4_hdr *iphdr = rte_pktmbuf_mtod_offset(
      pkt, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));

  uint8_t tos = iphdr->type_of_service;
  iphdr->type_of_service = (tos & 0xfc) | 0x03;
  iphdr->hdr_checksum = 0;
  iphdr->hdr_checksum = rte_ipv4_cksum(iphdr);

  return 0;
}

static void process_packet(struct rte_mbuf *packet) {
  struct rte_ether_hdr *eth_hdr;
  struct rte_ipv4_hdr *ip_hdr;
  struct rte_tcp_hdr *tcp_hdr;

  eth_hdr = rte_pktmbuf_mtod(packet, struct rte_ether_hdr *);
  if (rte_be_to_cpu_16(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV4) {
    ip_hdr =
        (struct rte_ipv4_hdr *)((char *)eth_hdr + sizeof(struct rte_ether_hdr));

    if (ip_hdr->next_proto_id == IPPROTO_TCP) {
      tcp_hdr = (struct rte_tcp_hdr *)((unsigned char *)ip_hdr +
                                       sizeof(struct rte_ipv4_hdr));

      uint32_t src_ip = rte_be_to_cpu_32(ip_hdr->src_addr);
      uint32_t dst_ip = rte_be_to_cpu_32(ip_hdr->dst_addr);
      uint16_t src_port = rte_be_to_cpu_16(tcp_hdr->src_port);
      uint16_t dst_port = rte_be_to_cpu_16(tcp_hdr->dst_port);
      uint8_t protocol = IPPROTO_TCP;

      fprintf(app.fp,
              "%u.%u.%u.%u:%u -> %u.%u.%u.%u:%u %lu %lu %lu %lu %lu %lu %lu "
              "%lu %lu %lu %lu %lu %lu %lu %lu %lu \n",
              (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
              (src_ip >> 8) & 0xFF, src_ip & 0xFF, src_port,
              (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
              (dst_ip >> 8) & 0xFF, dst_ip & 0xFF, dst_port, app.qlen[0][1],
              app.qlen[0][2], app.qlen[0][3], app.qlen[0][4], app.qlen[0][5],
              app.qlen[0][6], app.qlen[0][7], app.qlen[0][8], app.qlen[4][1],
              app.qlen[4][2], app.qlen[5][1], app.qlen[5][2], app.qlen[6][1],
              app.qlen[6][2], app.qlen[7][1], app.qlen[7][2]);
    }
  }
}

inline static int handle_ecn_marking(struct rte_mbuf *m, uint32_t port_id,
                                     uint32_t queue_id) {
#if QUERY_ECN_MARK == 1
  if (queue_id == 1)
#endif
    if (app.qlen[port_id][queue_id] > ECN_THRESHOLD) {
      mark_packet_with_ecn(m);
      return 1;
    }
  return 0;
}

inline static int check_buffer_threshold(uint32_t pkt_len, uint32_t port_id,
                                         uint32_t queue_id,
                                         uint32_t threshold) {
  return (pkt_len > app.total_remain ||
          app.occu_size_in[port_id][queue_id] + pkt_len >
              app.occu_size_out[port_id][queue_id] + threshold);
}

inline static void update_and_enqueue(struct rte_mbuf *m, uint32_t port_id,
                                      uint32_t queue_id, uint32_t pkt_len,
                                      int *ret) {
  *ret = rte_ring_sp_enqueue(app.rings_tx[port_id][queue_id], (void *)m);
  if (*ret == 0) {
    update_buffer_info(port_id, queue_id, pkt_len, true, false);
  }
}

inline static uint32_t get_dynamic_threshold(uint32_t queue_id) {
  uint32_t threshold = app.threshold;
#if SCHD == 1
  if (queue_id < 2) {
    threshold <<= HIGH_PRIO_SHIFT_ALPHA;
  } else {
    threshold <<= LOW_PRIO_SHIFT_ALPHA;
  }
#endif
  return threshold;
}

int packet_to_egress_cp(struct rte_mbuf *m, uint32_t port_id) {
  uint32_t queue_id = get_priority(m);
  uint32_t pkt_len = m->pkt_len;
  // complete partition
  uint32_t threshold = get_buffer_size() >> 3;

  if (check_buffer_threshold(pkt_len, port_id, queue_id, threshold)) {
    return 1;
  }

  handle_ecn_marking(m, port_id, queue_id);
  int ret;
  update_and_enqueue(m, port_id, queue_id, pkt_len, &ret);
  return ret;
}

// DT
int packet_to_egress(struct rte_mbuf *m, uint32_t port_id) {
  uint32_t queue_id = get_priority(m);
  uint32_t pkt_len = m->pkt_len;
  uint32_t threshold = get_dynamic_threshold(queue_id);

  if (check_buffer_threshold(pkt_len, port_id, queue_id, threshold)) {
    return 1;
  }

  handle_ecn_marking(m, port_id, queue_id);
  int ret;
  update_and_enqueue(m, port_id, queue_id, pkt_len, &ret);
  return ret;
}

int packet_to_egress_pushout(struct rte_mbuf *m, uint32_t port_id) {
  uint32_t queue_id = get_priority(m);
  uint32_t pkt_len = m->pkt_len;
  struct rte_mbuf *tofree;

  while (pkt_len > app.total_remain) {
    int max_len = 0, target_port_id = -1, target_queue_id = -1;

#if SCHD == 0
    // Find the longest queue
    for (uint32_t i = 0; i < app.n_ports; i++) {
      for (uint32_t j = 0; j < app.n_queues; j++) {
        int len = get_occu_size(i, j);
        if (len > max_len) {
          max_len = len;
          target_port_id = i;
          target_queue_id = j;
        }
      }
    }
#else
    // Push out lower priority queues
    for (uint32_t i = 0; i < app.n_ports; i++) {
      for (uint32_t j = 2; j < app.n_queues; j++) {
        int len = get_occu_size(i, j);
        if (len > max_len) {
          max_len = len;
          target_port_id = i;
          target_queue_id = j;
        }
      }
    }
#endif

    if (max_len > 0 && target_port_id != -1) {
      int ret = rte_ring_dequeue(app.rings_tx[target_port_id][target_queue_id],
                                 (void **)&tofree);
      if (ret == 0) {
        update_buffer_info(target_port_id, target_queue_id, tofree->pkt_len,
                           false, true);
        rte_pktmbuf_free(tofree);
      }
    } else {
      return 1;
    }
  }

  handle_ecn_marking(m, port_id, queue_id);
  int ret;
  update_and_enqueue(m, port_id, queue_id, pkt_len, &ret);
  return ret;
}

int packet_to_egress_occamy(struct rte_mbuf *m, uint32_t port_id) {
  uint32_t queue_id = get_priority(m);
  uint32_t pkt_len = m->pkt_len;
  uint32_t threshold =
      get_dynamic_threshold(queue_id) + app.occu_size_drop[port_id][queue_id];

  if (check_buffer_threshold(pkt_len, port_id, queue_id, threshold)) {
    return 1;
  }

  handle_ecn_marking(m, port_id, queue_id);
  int ret;
  update_and_enqueue(m, port_id, queue_id, pkt_len, &ret);
  return ret;
}

int packet_to_egress_abm(struct rte_mbuf *m, uint32_t port_id) {
  uint32_t queue_id = get_priority(m);
  uint32_t pkt_len = m->pkt_len;
  uint32_t threshold = app.threshold_abm[port_id][queue_id];

  if (check_buffer_threshold(pkt_len, port_id, queue_id, threshold)) {
    return 1;
  }

  handle_ecn_marking(m, port_id, queue_id);
  int ret;
  update_and_enqueue(m, port_id, queue_id, pkt_len, &ret);
  return ret;
}

inline static void apply_token_bucket(int *token, uint64_t *pre_tsc,
                                      const uint64_t sec_tsc) {
  uint64_t cur_tsc = rte_rdtsc();
  double inval = (cur_tsc - *pre_tsc) / (double)sec_tsc;
  double size = inval * SPEED_10G_LIMIT / 8; // Bytes
  int size_B = (int)(size);
  *token += size_B;
  if (*token > BUCKET_MAX_SIZE) {
    *token = BUCKET_MAX_SIZE;
  }
  *pre_tsc = cur_tsc;
}

inline static int dequeue_and_send_packet(
    uint32_t i, uint32_t j, int *token, uint64_t abm_sec_tsc,
    void (*update_info)(uint32_t, uint32_t, uint32_t, bool, bool)) {
  int ret =
      rte_ring_dequeue(app.rings_tx[i][j], (void **)app.mbuf_tx[i][j].array);
  if (ret != 0) {
    return ret;
  }

  uint32_t pkt_size = app.mbuf_tx[i][j].array[0]->pkt_len;
  if (app.method == 2) {
    update_aqm(i, j, pkt_size, abm_sec_tsc);
  }
  *token -= pkt_size;
  app.output_token_out[i] += (pkt_size + 199) / 200;
  update_info(i, j, pkt_size, false, false);

  uint32_t n_write;
  do {
    n_write = rte_eth_tx_burst(app.ports[i], 0, app.mbuf_tx[i][j].array, 1);
  } while (n_write == 0);

  return 0;
}

// Main Loop for TX
void app_main_loop_tx(uint32_t port_id,
                      void (*update_info)(uint32_t, uint32_t, uint32_t, bool,
                                          bool)) {
  uint32_t i = port_id;
  int *d_c = app.deflict_counter[i];
  int *quantum = app.quantum;
  const uint64_t sec_tsc = rte_get_tsc_hz();
  uint64_t abm_sec_tsc = sec_tsc / 1e5;
  uint64_t pre_tsc = rte_rdtsc();
  int token = 0;

  RTE_LOG(INFO, USER1, "Core %u is doing TX\n", rte_lcore_id());

  for (uint32_t j = 0;; j = (j + 1) % app.n_queues) {
    apply_token_bucket(&token, &pre_tsc, sec_tsc);
    if (token < 0) {
      continue;
    }

    if (!rte_ring_empty(app.rings_tx[i][j])) {
      d_c[j] += quantum[j];

      while (!rte_ring_empty(app.rings_tx[i][j])) {
        int ret =
            dequeue_and_send_packet(i, j, &token, abm_sec_tsc, update_info);
        if (ret != 0) {
          break;
        }

        d_c[j] -= app.mbuf_tx[i][j].array[0]->pkt_len;
        if (d_c[j] <= 0) {
          break;
        }
      }

      if (rte_ring_empty(app.rings_tx[i][j])) {
        d_c[j] = 0;
      }
    }
  }
}

// Main Loop for TX (SP)
void app_main_loop_tx_sp(uint32_t port_id,
                         void (*update_info)(uint32_t, uint32_t, uint32_t, bool,
                                             bool)) {
  uint32_t i = port_id;
  const uint64_t sec_tsc = rte_get_tsc_hz();
  uint64_t abm_sec_tsc = sec_tsc / 1e5;
  uint64_t pre_tsc = rte_rdtsc();
  int token = 0;

  RTE_LOG(INFO, USER1, "Core %u is doing TX(sp)\n", rte_lcore_id());

  while (true) {
    uint32_t j;
    for (j = 0; j < app.n_queues; j++) {
      if (!rte_ring_empty(app.rings_tx[i][j])) {
        break;
      }
    }

    apply_token_bucket(&token, &pre_tsc, sec_tsc);
    if (token < 0 || j == app.n_queues) {
      continue;
    }

    dequeue_and_send_packet(i, j, &token, abm_sec_tsc, update_info);
  }
}

void update_aqm(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len,
                uint64_t sec_tsc) {

  uint64_t cur_tsc = rte_rdtsc();
  if (cur_tsc - app.port_abm_info[port_id].start_tsc > sec_tsc) {
    app.abm_info[port_id].num_of_byte = app.port_abm_info[port_id].num_of_byte;
    memcpy(app.abm_info[port_id].queue_abm_info,
           app.port_abm_info[port_id].queue_abm_info,
           sizeof(app.port_abm_info[port_id].queue_abm_info));
    app.port_abm_info[port_id].start_tsc = cur_tsc;
    memset(app.port_abm_info[port_id].queue_abm_info, 0,
           sizeof(app.port_abm_info[port_id].queue_abm_info));
    app.port_abm_info[port_id].num_of_byte = 0;
  }
  app.port_abm_info[port_id].num_of_byte += pkt_len;
  app.port_abm_info[port_id].queue_abm_info[queue_id] += pkt_len;
}

void app_main_loop_update_T(void) {
  RTE_LOG(INFO, USER1, "Core %u is doing ABM\n", rte_lcore_id());
  uint64_t cur_tsc, last_tsc, sec_tsc;
  uint32_t i, j;
  uint32_t np[APP_MAX_QUEUES];
  int alpha = app.dt_shift_alpha;
  while (true) {
    int shared_used = 0;
    for (i = 0; i < APP_MAX_PORTS; i++) {
      for (j = 0; j < APP_MAX_QUEUES; j++) {
        int tmp = get_occu_size(i, j);
        app.qlen[i][j] = tmp;
        shared_used += get_occu_size(i, j);
      }
    }
    long long rem = ((long long)get_buffer_size() - (long long)shared_used);
    long long threshold;
    if (rem < 0) {
      rem = 0;
      app.total_remain = 0;
    } else
      app.total_remain = rem;

    // get np for abm
    for (i = 0; i < APP_MAX_QUEUES; i++) {
      np[i] = 0;
      for (j = 0; j < APP_MAX_PORTS; j++) {
        if (get_occu_size(j, i) > 0.9 * app.T[j][i] + (double)app.reserve) {
          np[i]++;
        }
      }
      if (np[i] == 0)
        np[i] = 1;
    }

    for (i = 0; i < APP_MAX_PORTS; i++) {
      for (j = 0; j < APP_MAX_QUEUES; j++) {
#if SCHD == 1
        if (j >= 2)
          threshold = rem << LOW_PRIO_SHIFT_ALPHA; // low prio
        else
          threshold = rem << HIGH_PRIO_SHIFT_ALPHA; // high prio
#else
        // alpha * (B - Q(t))
        threshold = alpha >= 0 ? (rem << alpha) : (rem >> -alpha);
#endif
        // get aqm arguments min(1, max(1/8, ))
        double aqm;
        double min_aqm = 1 / (double)app.n_queues;
        if (app.abm_info[i].num_of_byte == 0) {
          aqm = min_aqm;
        } else {
          aqm = app.abm_info[i].queue_abm_info[j] /
                (double)app.abm_info[i].num_of_byte;
          if (aqm < min_aqm)
            aqm = min_aqm;
        }
        if (aqm > 1)
          aqm = 1;

        // update T
        app.T[i][j] = (double)threshold / (double)np[j] * aqm;
        app.threshold_abm[i][j] = (int)app.T[i][j];
      }
    }
  }
}

static long long get_token_size(long long token_size_in) {
  // sum of tx used token
  uint64_t token_out_sum = 0;
  for (int i = 0; i < APP_MAX_PORTS; i++) {
    token_out_sum += app.output_token_out[i];
  }

  return token_size_in - token_out_sum;
}

void app_main_loop_headdrop(void) {
  RTE_LOG(INFO, USER1, "Core %u is doing HEADDROP\n", rte_lcore_id());
  uint64_t cur_tsc, last_tsc, sec_tsc, pre_tsc, last_print_tsc;
  last_tsc = rte_rdtsc();
  sec_tsc = rte_get_tsc_hz();
  pre_tsc = rte_rdtsc();
  last_print_tsc = rte_rdtsc();
  struct rte_mbuf *m;
  int i, k, flg, ret, last_i, last_k;
  i = 0, k = 0, last_i = 0, last_k = 0;
  uint64_t drop_counter = 0;

  uint64_t total_tsc_interval = 0;
  long long headdrop_token = 0;

  int continue_c = 0;
  while (true) {
    cur_tsc = rte_rdtsc();
    total_tsc_interval += cur_tsc - pre_tsc;
    pre_tsc = cur_tsc;
    if (total_tsc_interval > sec_tsc / 2e6) {
      double total_interval = total_tsc_interval / (double)sec_tsc;
      long long token_size = get_token_size(headdrop_token);
      if (token_size < HEADDROP_BUCKET_MAX_SIZE) {
        if (HEADDROP_BUCKET_MAX_SIZE - token_size <
            (int)(SWITCH_MAX_FREQ * total_interval))
          headdrop_token += HEADDROP_BUCKET_MAX_SIZE - token_size;
        else
          headdrop_token += (int)(SWITCH_MAX_FREQ * total_interval);
      }
      total_tsc_interval = 0;
    }

    if (get_token_size(headdrop_token) < 0) {
      continue;
    }
    flg = 1;
    for (i = last_i; i < app.n_ports; i++) {
      for (k = ((flg && i == last_i) ? last_k + 1 : 0); k < app.n_queues; k++) {
        flg = 0;
        uint32_t threshold = app.threshold;
#if SCHD == 1
        if (k < 2)
          threshold = threshold << HIGH_PRIO_SHIFT_ALPHA;
#endif
        if (app.occu_size_in[i][k] >
            app.occu_size_out[i][k] + app.occu_size_drop[i][k] + threshold)

        {
          ret = rte_ring_dequeue(app.rings_tx[i][k], (void **)&m);
          if (ret != 0) {
            continue;
          }
          update_buffer_info(i, k, m->pkt_len, false, true);
          // if (likely(headdrop_token > (m->pkt_len + 199) / 200))
          headdrop_token -= (m->pkt_len + 199) / 200;
          // else
          // {
          //     headdrop_token = 0;
          // }
          rte_pktmbuf_free(m);
          app.drop[i][k]++;
          last_i = i;
          last_k = k;
          goto next_time;
        }
      }
    }
  next_time:
    if (i != app.n_ports)
      last_tsc = cur_tsc;
    else {
      last_i = 0, last_k = 0;
    }
  }
}

void app_main_loop_headdrop_longest(void) {
  RTE_LOG(INFO, USER1, "Core %u is doing HEADDROP(longest)\n", rte_lcore_id());
  uint64_t cur_tsc, last_tsc, sec_tsc, pre_tsc, last_print_tsc;
  last_tsc = rte_rdtsc();
  sec_tsc = rte_get_tsc_hz();
  pre_tsc = rte_rdtsc();
  last_print_tsc = rte_rdtsc();
  struct rte_mbuf *m;
  int i, k, flg, ret, last_i, last_k;
  i = 0, k = 0, last_i = 0, last_k = 0;
  uint64_t drop_counter = 0;

  uint64_t total_tsc_interval = 0;
  long long headdrop_token = 0;

  int continue_c = 0;
  // limited frequency to freq
  while (true) {
    cur_tsc = rte_rdtsc();
    total_tsc_interval += cur_tsc - pre_tsc;
    pre_tsc = cur_tsc;
    if (total_tsc_interval > sec_tsc / 2e6) {
      double total_interval = total_tsc_interval / (double)sec_tsc;
      long long token_size = get_token_size(headdrop_token);
      if (token_size < HEADDROP_BUCKET_MAX_SIZE) {
        if (HEADDROP_BUCKET_MAX_SIZE - token_size <
            (int)(SWITCH_MAX_FREQ * total_interval))
          headdrop_token += HEADDROP_BUCKET_MAX_SIZE - token_size;
        else
          headdrop_token += (int)(SWITCH_MAX_FREQ * total_interval);
      }
      total_tsc_interval = 0;
    }
    if (get_token_size(headdrop_token) < 0) {
      continue;
    }
    flg = 1;

    int target_port_id, target_queue_id, max_qlen;
    target_port_id = -1;
    target_queue_id = -1;
    max_qlen = 0;

    for (i = 0; i < app.n_ports; i++) {
      for (k = 0; k < app.n_queues; k++) {
        uint32_t threshold = app.threshold;
        int tmp = threshold > max_qlen ? threshold : max_qlen;
        if (app.occu_size_in[i][k] >
            app.occu_size_out[i][k] + app.occu_size_drop[i][k] + tmp) {
          target_port_id = i;
          target_queue_id = k;
          int len = get_occu_size(i, k);
          if (len > max_qlen) {
            max_qlen = len;
          }
        }
      }
    }
    if (max_qlen > 0 && target_port_id != -1) {
      ret = rte_ring_dequeue(app.rings_tx[target_port_id][target_queue_id],
                             (void **)&m);
      if (ret != 0) {
        continue;
      }
      update_buffer_info(target_port_id, target_queue_id, m->pkt_len, false,
                         true);
      headdrop_token -= (m->pkt_len + 199) / 200;
    } else
      continue;
    last_tsc = cur_tsc;
  }
}
