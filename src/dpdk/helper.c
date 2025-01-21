#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
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
#include <rte_interrupts.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_lpm.h>
#include <rte_lpm6.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_string_fns.h>
#include <rte_tcp.h>

#include "main.h"

#define INFO_DEBUG 0

uint32_t get_priority(struct rte_mbuf *m) {
  struct rte_tcp_hdr *tcp;
  // Get Ethernet header
  struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
  struct rte_ipv4_hdr *ip_hdr;
  // Make sure the packet contains the IP header
  if (!eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
    // put high priority
    return 1;
  }

  // Get IP header
  ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

  // Determine the IP protocol field
  if (ip_hdr->next_proto_id == IPPROTO_TCP) {
    // This is a TCP packet
    tcp = rte_pktmbuf_mtod_offset(m, struct rte_tcp_hdr *,
                                  sizeof(struct rte_ether_hdr) +
                                      sizeof(struct rte_ipv4_hdr));
    // Check the ACK flag
    if (((tcp->tcp_flags & RTE_TCP_ACK_FLAG) != 0) && m->pkt_len < 100) {
      return 0;
    }
  }
  // Get type of service(ToS) field
  uint8_t tos = ip_hdr->type_of_service;
  // printf("tos = %u\n", tos);
  // get dscp field
  uint8_t dscp = (tos >> 2) & 0x3f;
  return (uint32_t)dscp >= APP_MAX_QUEUES ? 1 : (uint32_t)dscp;
}

// this loop is to calculate the dt threshold to improve performace
void app_main_loop_threshold(void) {
  RTE_LOG(INFO, USER1, "Core %u is caculating threshold\n", rte_lcore_id());
  uint32_t i, j;
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
    long long rst = ((long long)get_buffer_size() - (long long)shared_used);
    if (rst < 0) {
      rst = 0;
      app.total_remain = 0;
      app.threshold = 0;
    }

    app.total_remain = rst;
#if SCHD == 0
    if (alpha > 0)
      app.threshold = (rst << alpha);
    else
      app.threshold = (rst >> (-alpha));
#else
    app.threshold = rst;
#endif
  }
}

static void printinfo_to_logfile(uint64_t cur_tsc) {
  if (app.method != 2)
    fprintf(app.fp,
            "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
            "%lu %lu %lu %lu %lu %lu %lu %lu %lu %u\n",
            cur_tsc, get_occu_size(0, 0), get_occu_size(0, 1),
            get_occu_size(0, 2), get_occu_size(1, 0), get_occu_size(1, 1),
            get_occu_size(1, 2), get_occu_size(2, 0), get_occu_size(2, 1),
            get_occu_size(2, 2), get_occu_size(3, 0), get_occu_size(3, 1),
            get_occu_size(3, 2),

            get_occu_size(4, 0), get_occu_size(4, 1), get_occu_size(4, 2),
            get_occu_size(5, 0), get_occu_size(5, 1), get_occu_size(5, 2),
            get_occu_size(6, 0), get_occu_size(6, 1), get_occu_size(6, 2),
            get_occu_size(7, 0), get_occu_size(7, 1), get_occu_size(7, 2),

            app.threshold);
  else
    fprintf(app.fp,
            "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
            "%lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d "
            "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            cur_tsc, get_occu_size(0, 0), get_occu_size(0, 1),
            get_occu_size(0, 2), get_occu_size(1, 0), get_occu_size(1, 1),
            get_occu_size(1, 2), get_occu_size(2, 0), get_occu_size(2, 1),
            get_occu_size(2, 2), get_occu_size(3, 0), get_occu_size(3, 1),
            get_occu_size(3, 2),

            get_occu_size(4, 0), get_occu_size(4, 1), get_occu_size(4, 2),
            get_occu_size(5, 0), get_occu_size(5, 1), get_occu_size(5, 2),
            get_occu_size(6, 0), get_occu_size(6, 1), get_occu_size(6, 2),
            get_occu_size(7, 0), get_occu_size(7, 1), get_occu_size(7, 2),

            app.threshold_abm[0][0], app.threshold_abm[0][1],
            app.threshold_abm[0][2], app.threshold_abm[1][0],
            app.threshold_abm[1][1], app.threshold_abm[1][2],
            app.threshold_abm[2][0], app.threshold_abm[2][1],
            app.threshold_abm[2][2], app.threshold_abm[3][0],
            app.threshold_abm[3][1], app.threshold_abm[3][2],

            app.threshold_abm[4][0], app.threshold_abm[4][1],
            app.threshold_abm[4][2], app.threshold_abm[5][0],
            app.threshold_abm[5][1], app.threshold_abm[5][2],
            app.threshold_abm[6][0], app.threshold_abm[6][1],
            app.threshold_abm[6][2], app.threshold_abm[7][0],
            app.threshold_abm[7][1], app.threshold_abm[7][2]);
}

void app_main_loop_printinfo(void) {
  RTE_LOG(INFO, USER1, "Core %u is doing DEBUG\n", rte_lcore_id());
  uint64_t cur_tsc, last_tsc, sec_tsc;
  last_tsc = rte_rdtsc();
  sec_tsc = rte_get_tsc_hz();
  sec_tsc = rte_get_tsc_hz() / 1e5;
  return;

  while (true) {
    cur_tsc = rte_rdtsc();
    if (cur_tsc < last_tsc || cur_tsc >= sec_tsc + last_tsc) {
      // 4 8 C 10 14
      // printf("time %lu occu_size(8) %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu
      // %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu  T %u po %lu
      // abm %d drop(%lu %lu %lu[%lu] %lu %lu %lu[%lu] %lu %lu %lu[%lu] %lu %lu
      // %lu[%lu] )\n",
      //        cur_tsc,
      //        get_occu_size(0, 0), get_occu_size(0, 1), get_occu_size(0, 8),
      //        get_occu_size(1, 0), get_occu_size(1, 1), get_occu_size(1, 2),
      //        get_occu_size(2, 0), get_occu_size(2, 1), get_occu_size(2, 2),
      //        get_occu_size(3, 0), get_occu_size(3, 1), get_occu_size(3, 2),
      //        get_occu_size(4, 0), get_occu_size(4, 1), get_occu_size(4, 2),
      //        get_occu_size(5, 0), get_occu_size(5, 1), get_occu_size(5, 2),
      //        get_occu_size(6, 0), get_occu_size(6, 1), get_occu_size(6, 2),
      //        get_occu_size(7, 0), get_occu_size(7, 1), get_occu_size(7, 2),
      //        app.threshold, 1, app.threshold_abm[0][1],
      //        app.drop[0][0], app.drop[0][1], app.drop[0][2], app.scc[0][1],
      //        app.drop[1][0], app.drop[1][1], app.drop[1][2], app.scc[1][1],
      //        app.drop[2][0], app.drop[2][1], app.drop[2][2], app.scc[2][1],
      //        app.drop[3][0], app.drop[3][1], app.drop[3][2], app.scc[3][1]);
      double aqm = app.abm_info[0].queue_abm_info[2] /
                   (double)app.abm_info[0].num_of_byte;

      if (app.method != 2)
        fprintf(app.fp,
                "time %lu occu_size(8) %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
                "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
                "%lu %lu %lu %lu %lu %lu %lu T %u (%lu %lu[%lu] %lu %lu[%lu] "
                "%lu %lu[%lu] %lu %lu[%lu] )\n",
                cur_tsc, get_occu_size(0, 0), get_occu_size(0, 1),
                get_occu_size(0, 2), get_occu_size(0, 3), get_occu_size(1, 0),
                get_occu_size(1, 1), get_occu_size(1, 2), get_occu_size(1, 3),
                get_occu_size(2, 0), get_occu_size(2, 1), get_occu_size(2, 2),
                get_occu_size(2, 3), get_occu_size(3, 0), get_occu_size(3, 1),
                get_occu_size(3, 2), get_occu_size(3, 3),

                get_occu_size(4, 0), get_occu_size(4, 1), get_occu_size(4, 2),
                get_occu_size(4, 3), get_occu_size(5, 0), get_occu_size(5, 1),
                get_occu_size(5, 2), get_occu_size(5, 3), get_occu_size(6, 0),
                get_occu_size(6, 1), get_occu_size(6, 2), get_occu_size(6, 3),
                get_occu_size(7, 0), get_occu_size(7, 1), get_occu_size(7, 2),
                get_occu_size(7, 3),

                app.threshold, app.drop[0][0], app.drop[0][1], app.scc[0][1],
                app.drop[1][0], app.drop[1][1], app.scc[1][1], app.drop[2][0],
                app.drop[2][1], app.scc[2][1], app.drop[3][0], app.drop[3][1],
                app.scc[3][1]);
      else
        fprintf(app.fp,
                "time %lu occu_size(8) %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
                "%lu %lu %lu %lu %lu %lu T %lf %lf %lf %lf %lf %lf %lf %lf %lf "
                "%lu %lu %lu %lu %lu %lu %lu %lu %lu (%lu %lu[%lu] %lu "
                "%lu[%lu] %lu %lu[%lu] %lu %lu[%lu] )\n",
                cur_tsc, get_occu_size(0, 0), get_occu_size(0, 1),
                get_occu_size(0, 2), get_occu_size(0, 3), get_occu_size(1, 0),
                get_occu_size(1, 1), get_occu_size(1, 2), get_occu_size(1, 3),
                get_occu_size(2, 0), get_occu_size(2, 1), get_occu_size(2, 2),
                get_occu_size(2, 3), get_occu_size(3, 0), get_occu_size(3, 1),
                get_occu_size(3, 2), get_occu_size(3, 3), app.T[0][2],
                app.T[0][1], app.T[2][1], app.T[3][1], app.T[4][1], app.T[5][1],
                app.T[6][1], app.T[7][1], aqm,
                app.abm_info[0].queue_abm_info[0],
                app.abm_info[0].queue_abm_info[1],
                app.abm_info[0].queue_abm_info[2],
                app.abm_info[0].queue_abm_info[3],
                app.abm_info[0].queue_abm_info[4],
                app.abm_info[0].queue_abm_info[5],
                app.abm_info[0].queue_abm_info[6],
                app.abm_info[0].queue_abm_info[7], app.abm_info[0].num_of_byte,
                app.drop[0][0], app.drop[0][1], app.scc[0][1], app.drop[1][0],
                app.drop[1][1], app.scc[1][1], app.drop[2][0], app.drop[2][1],
                app.scc[2][1], app.drop[3][0], app.drop[3][1], app.scc[3][1]);
      // fprintf(app.fp, "sum_size = %lu\n", get_totoccu());

      // printinfo_to_logfile(cur_tsc);
      last_tsc = cur_tsc;
    }
  }
}
