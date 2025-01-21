/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

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
#include <rte_tcp.h>

#include "main.h"

static void signal_handler(int signum) {
  uint32_t i;

  if (signum != SIGINT && signum != SIGTERM)
    return;

  printf("\nSignal %d received, preparing to exit...\n", signum);
  fclose(app.fp);
  exit(0);
}

void app_main_loop_tx_handle(uint32_t port_id) {
  buffer_info_func_t buffer_info_func = NULL;

  if (app.method == 0 || app.method == 1 || app.method == 2 ||
      app.method == 3 || app.method == 4 || app.method == 5) {
    buffer_info_func = update_buffer_info;
  } else {
    rte_panic("method illegal\n");
  }
#if SCHD == 0
  app_main_loop_tx(port_id, buffer_info_func);
#else
  app_main_loop_tx_sp(port_id, buffer_info_func);
#endif
}

int main(int argc, char **argv) {
  uint32_t lcore;
  int ret;

  /* Init EAL */
  ret = rte_eal_init(argc, argv);
  if (ret < 0)
    return -1;
  argc -= ret;
  argv += ret;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Parse application arguments (after the EAL ones) */
  ret = app_parse_args(argc, argv);
  if (ret < 0) {
    app_print_usage();
    return -1;
  }

  /* Init */
  app_init();

  /* Launch per-lcore init on every lcore */
  rte_eal_mp_remote_launch(app_lcore_main_loop, NULL, CALL_MAIN);
  RTE_LCORE_FOREACH_WORKER(lcore) {
    if (rte_eal_wait_lcore(lcore) < 0)
      return -1;
  }

  return 0;
}

int app_lcore_main_loop(__attribute__((unused)) void *arg) {
  unsigned lcore;

  lcore = rte_lcore_id();

  uint32_t i;
  for (i = 0; i < app.n_ports; i++) {
    if (lcore == app.core_tx[i]) {
      app_main_loop_tx_handle(i);
      return 0;
    }
  }
  if (lcore == app.core_worker) {
    if (app.method == 3) {
      // pushout
      app_main_loop_worker(packet_to_egress_pushout);
    } else if (app.method == 2) {
      // abm
      app_main_loop_worker(packet_to_egress_abm);
    } else if (app.method == 1 || app.method == 5)
      // occamy
      app_main_loop_worker(packet_to_egress_occamy);
    else if (app.method == 0)
      // dt
      app_main_loop_worker(packet_to_egress);
    else if (app.method == 4)
      // complete partition
      app_main_loop_worker(packet_to_egress_cp);
    return 0;
  }

  if (lcore == app.core_rx) {
    app_main_loop_rx();
    return 0;
  }

  if (lcore == app.core_func) {
    if (app.method == 2) {
      // abm
      app_main_loop_update_T();
    } else if (app.method == 1) {
      // occamy
      app_main_loop_headdrop();
    } else if (app.method == 5) {
      // longest occamy
      app_main_loop_headdrop_longest();
    }

    return 0;
  }

  if (lcore == app.core_debug) {
    app_main_loop_printinfo();
    return 0;
  }

  if (lcore == app.core_threshold) {
    if (app.method == 2) {
      return 0;
    }
    app_main_loop_threshold();
    return 0;
  }

  // if (lcore == app.core_update_aqm)
  // {
  //     app_main_loop_update_aqm();
  //     return 0;
  // }

  // if (lcore == app.core_free)
  // {
  //     app_main_loop_free();
  //     return 0;
  // }

  return 0;
}
