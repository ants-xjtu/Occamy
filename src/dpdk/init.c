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
#include <rte_hash.h>
#include <rte_hash_crc.h>
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

struct app_params app = {
    /* Ports*/
    .n_ports = APP_MAX_PORTS,

    .n_queues = APP_MAX_QUEUES,

    /* Rings */
    .ring_rx_size = 4096 << 3,
    .ring_tx_size = 4096 << 3,
    .ring_free_size = 4096,

    /* Buffer pool */
    .pool_buffer_size = 2048 + RTE_PKTMBUF_HEADROOM,
    .pool_size = 8 * 1024 * 1024,
    .pool_cache_size = 256,

    /* Burst sizes */
    .burst_size_rx_read = 16,
    .burst_size_rx_write = 16,
    .burst_size_worker_read = 16,
    .burst_size_worker_write = 16,
    .burst_size_tx_read = 1,
    .burst_size_tx_write = 64,
};

static struct rte_eth_conf port_conf = {
    .rxmode =
        {
            // .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
            // .split_hdr_size = 0,
            .offloads = RTE_ETH_RX_OFFLOAD_CHECKSUM,
        },
    // .rx_adv_conf = {
    //     .rss_conf = {
    //         .rss_key = NULL,
    //         .rss_hf = RTE_ETH_RSS_IP,
    //     },
    // },
    .txmode =
        {
            .mq_mode = RTE_ETH_MQ_TX_NONE,
        },
};

static struct rte_eth_rxconf rx_conf = {
    .rx_thresh =
        {
            .pthresh = 8,
            .hthresh = 8,
            .wthresh = 4,
        },
    .rx_free_thresh = 64,
    .rx_drop_en = 0,
};

static struct rte_eth_txconf tx_conf = {
    .tx_thresh =
        {
            .pthresh = 36,
            .hthresh = 0,
            .wthresh = 0,
        },
    .tx_free_thresh = 0,
    .tx_rs_thresh = 0,
};
#define MAX_LINE_LENGTH                                                        \
  18 // Max length of MAC address line (including newline and null terminator)

/**
 * Function to convert MAC address string to rte_ether_addr structure.
 */
int convert_mac_str_to_rte_ether_addr(uint32_t port_id, const char *mac_str,
                                      struct rte_ether_addr *ether_addr) {
  int values[6];
  if (6 == sscanf(mac_str, "%x:%x:%x:%x:%x:%x%*c", &values[0], &values[1],
                  &values[2], &values[3], &values[4], &values[5])) {
    for (int i = 0; i < 6; ++i)
      ether_addr->addr_bytes[i] = (uint8_t)values[i];
    int index = rte_hash_add_key(app.l2_hash, ether_addr);
    app.fwd_table[index] = port_id;
    return 0; // Success
  }
  return -1; // Failure
}

/**
 * Function to read MAC addresses from a file and convert them to rte_ether_addr
 * structures.
 */
void read_mac_addresses_from_file(const char *file_name) {
  FILE *file = fopen(file_name, "r");
  if (file == NULL) {
    perror("Error opening file");
    return;
  }

  char line[MAX_LINE_LENGTH];
  int index = 0;
  while (fgets(line, sizeof(line), file)) {
    struct rte_ether_addr ether_addr;
    if (convert_mac_str_to_rte_ether_addr(index, line, &ether_addr) == 0) {
      // Successfully converted, do something with ether_addr
      printf("port %u MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", index,
             ether_addr.addr_bytes[0], ether_addr.addr_bytes[1],
             ether_addr.addr_bytes[2], ether_addr.addr_bytes[3],
             ether_addr.addr_bytes[4], ether_addr.addr_bytes[5]);
      index++;
    }
  }

  fclose(file);
}

static void app_init_params(void) {
  uint32_t i, j;
  for (i = 0; i < APP_MAX_PORTS; i++) {
    app.mbuf_rx[i].n_mbufs = app.mbuf_wk[i].n_mbufs = 0;
    for (j = 0; j < APP_MAX_QUEUES; j++) {
      app.mbuf_tx[i][j].n_mbufs = 0;
    }
    app.mbuf_tx_test[i].n_mbufs = 0;

    // app.mbuf_free[i].n_mbufs = 0;
  }
  app.mbuf_free.n_mbufs = 0;
  /* init token bucket info */
  for (i = 0; i < APP_MAX_PORTS; i++) {
    app.bucket[i].pre_tsc = rte_rdtsc();
    app.bucket[i].token = 0;
  }
  memset(app.output_token_out, 0, sizeof app.output_token_out);

  /* init DRR info*/
  for (i = 0; i < APP_MAX_PORTS; i++) {
    for (j = 0; j < APP_MAX_QUEUES; j++) {
      app.deflict_counter[i][j] = 0;
    }
  }
  for (i = 0; i < APP_MAX_QUEUES; i++)
    app.quantum[i] = 2000;

  /* init buffer size info */
  // 419,430 5.12per-port-per-Gbps
  app.buffer_size = 419430;
  // app.buffer_size = 2 * 1024 * 1024;
  // app.buffer_size = 786432;
  app.reserve = 0;
  app.total_rsrv = app.reserve * app.n_ports * app.n_queues;
  app.total_remain = app.buffer_size;
  app.pri_shift_alpha = 7;
  app.threshold = 0;
  app.shared_used_in = 0;
  app.shared_used_out = 0;
  app.tot_occu_in = app.tot_occu_out = 0;
  for (i = 0; i < APP_MAX_PORTS; i++) {
    for (j = 0; j < APP_MAX_QUEUES; j++) {
      app.occu_size_in[i][j] = 0;
      app.occu_size_out[i][j] = 0;
      app.occu_size_drop[i][j] = 0;

      app.qlen[i][j] = 0;
    }
  }

  for (i = 0; i < APP_MAX_QUEUES; i++) {
    app.shared_used_queue_in[i] = 0;
    app.shared_used_queue_out[i] = 0;
  }

  /* init abm info */
  for (i = 0; i < APP_MAX_PORTS; i++) {
    app.abm_info[i].num_of_byte = 0;
    app.abm_info[i].start_tsc = 0;
    app.port_abm_info[i].num_of_byte = 0;
    app.port_abm_info[i].start_tsc = 0;
    for (j = 0; j < APP_MAX_QUEUES; j++) {
      app.abm_info[i].queue_abm_info[j] = 0;
      app.port_abm_info[i].queue_abm_info[j] = 0;
      app.T[i][j] = 0.0;
      app.threshold_abm[i][j] = 0;
    }
  }

  /* init debug info */
  for (int i = 0; i < APP_MAX_PORTS; i++)
    for (int j = 0; j < APP_MAX_QUEUES; j++) {
      app.drop[i][j] = 0;
      app.scc[i][j] = 0;
    }

  /* init hash func */
  char name[10] = "hash_name";
  struct rte_hash_parameters hash_params = {
      .name = name,
      .entries = FORWARD_ENTRY,
      .key_len = sizeof(struct rte_ether_addr),
      .hash_func = rte_hash_crc,
      .hash_func_init_val = 0,
  };
  app.l2_hash = rte_hash_create(&hash_params);

  /* static routing table */
  read_mac_addresses_from_file(app.mac_file);

  /* init logfile */
  app.fp = fopen("/dev/shm/logfile.txt", "w");
}

static void app_init_mbuf_pools(void) {
  /* Init the buffer pool */
  RTE_LOG(INFO, USER1, "Creating the mbuf pool ...\n");
  app.pool =
      rte_pktmbuf_pool_create("mempool", app.pool_size, app.pool_cache_size, 0,
                              app.pool_buffer_size, rte_socket_id());
  if (app.pool == NULL)
    rte_panic("Cannot create mbuf pool\n");
}

static void app_init_rings(void) {
  uint32_t i, j;

  /* init RX rings */
  for (i = 0; i < app.n_ports; i++) {
    char name[32];

    snprintf(name, sizeof(name), "app_ring_rx_%u", i);

    app.rings_rx[i] = rte_ring_create(name, app.ring_rx_size, rte_socket_id(),
                                      RING_F_SP_ENQ | RING_F_SC_DEQ);

    if (app.rings_rx[i] == NULL)
      rte_panic("Cannot create RX ring %u\n", i);
  }

  /* init TX rings */
  for (i = 0; i < app.n_ports; i++) {

    for (j = 0; j < app.n_queues; j++) {
      char name[32];

      snprintf(name, sizeof(name), "app_ring_tx_%u_%u", i, j);

      app.rings_tx[i][j] =
          rte_ring_create(name, app.ring_tx_size, rte_socket_id(),
                          RING_F_SP_ENQ | RING_F_MC_RTS_DEQ);

      if (app.rings_tx[i][j] == NULL)
        rte_panic("Cannot create TX ring %u %u\n", i, j);
    }
  }

  /* init free ring */
  app.rings_to_free =
      rte_ring_create("app_ring_to_free", app.ring_tx_size, rte_socket_id(),
                      RING_F_SP_ENQ | RING_F_MC_RTS_DEQ);
  if (app.rings_to_free == NULL)
    rte_panic("Cannot create free ring\n");
}

static void app_ports_check_link(void) {
  uint32_t all_ports_up, i;

  all_ports_up = 1;
  for (i = 0; i < app.n_ports; i++) {
    struct rte_eth_link link;
    uint16_t port;

    port = app.ports[i];
    memset(&link, 0, sizeof(link));
    rte_eth_link_get_nowait(port, &link);
    RTE_LOG(INFO, USER1, "Port %u (%u Gbps) %s\n", port, link.link_speed / 1000,
            link.link_status ? "UP" : "DOWN");

    if (link.link_status == RTE_ETH_LINK_DOWN)
      all_ports_up = 0;
  }

  if (all_ports_up == 0)
    printf("Some NIC ports are DOWN\n");
}

static void app_init_ports(void) {
  uint32_t i, j;

  /* Init NIC ports, then start the ports */
  for (i = 0; i < app.n_ports; i++) {
    uint16_t port;
    int ret;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_conf local_port_conf = port_conf;

    port = app.ports[i];
    // RTE_LOG(INFO, USER1, "Initializing NIC port %u ...\n", port);

    // /* Modify RSS hash function */
    // rte_eth_dev_info_get(port, &dev_info);

    // local_port_conf.rx_adv_conf.rss_conf.rss_hf &=
    //     dev_info.flow_type_rss_offloads;
    // if (local_port_conf.rx_adv_conf.rss_conf.rss_hf !=
    //     port_conf.rx_adv_conf.rss_conf.rss_hf)
    // {
    //     printf("Port %u modified RSS hash function based on hardware
    //     support,"
    //            "requested:%#" PRIx64 " configured:%#" PRIx64 "\n",
    //            port,
    //            port_conf.rx_adv_conf.rss_conf.rss_hf,
    //            local_port_conf.rx_adv_conf.rss_conf.rss_hf);
    // }

    /* Init port */
    ret = rte_eth_dev_configure(port, 1, 1, &local_port_conf);
    if (ret < 0)
      printf("Cannot init NIC port %u (%d)\n", port, ret);

    rte_eth_promiscuous_enable(port);

    rte_eth_macaddr_get(port, &app.port_eth_addr[port]);

    /* Init RX queues */
    ret = rte_eth_rx_queue_setup(port, 0, 256, rte_eth_dev_socket_id(port),
                                 &rx_conf, app.pool);
    if (ret < 0)
      rte_panic("Cannot init RX for port %u (%d)\n", (uint32_t)port, ret);

    /* Init TX queues */
    ret = rte_eth_tx_queue_setup(port, 0, 64, rte_eth_dev_socket_id(port),
                                 &tx_conf);
    if (ret < 0)
      rte_panic("Cannot init TX for port %u (%d)\n", (uint32_t)port, ret);

    /* Start port */
    ret = rte_eth_dev_start(port);
    if (ret < 0)
      rte_panic("Cannot start port %u (%d)\n", port, ret);
  }

  sleep(5);

  app_ports_check_link();
}

// print switch information
void app_print_switchinfo() {
  printf(
      "====================DPDK l2-self-learning Switch===================\n");
  printf("+ num of ports:           %-40u+\n", app.n_ports);
  printf("+ num of queues:          %-40u+\n", app.n_queues);
  printf("+ buffer_size:            %-40lu+\n", get_buffer_size());
  printf("+ private size:           %-40lu+\n", app.reserve);
  if (app.dt_shift_alpha >= 0)
    printf("+ dt-alpha:               %-40d+\n",
           app.dt_shift_alpha >= 0 ? (1 << app.dt_shift_alpha)
                                   : (1 << (-app.dt_shift_alpha)));
  else
    printf("+ dt-alpha:             1/%-40d+\n",
           app.dt_shift_alpha >= 0 ? (1 << app.dt_shift_alpha)
                                   : (1 << (-app.dt_shift_alpha)));
  if (ENABLE_ECN)
    printf("+ ECN-mark:               %-40s+\n", "ON");
  else
    printf("+ ECN-mark:               %-40s+\n", "OFF");
  if (app.method == 0)
    printf("+ method:                 %-40s+\n", "DT");
  else if (app.method == 1) {
    printf("+ method:                 %-40s+\n", "occamy");
  } else if (app.method == 5) {
    printf("+ method:                 %-40s+\n", "occamy-longest");
  } else if (app.method == 3)
    printf("+ method:                 %-40s+\n", "pushout");
  else if (app.method == 2)
    printf("+ method:                 %-40s+\n", "abm");
  else if (app.method == 4)
    printf("+ method:                 %-40s+\n", "ST(complete partition)");
#if SCHD == 1
  printf("+ schedule method:        %-40s+\n", "SP(PQ) strict priority");
#else
  printf("+ schedule method:        %-40s+\n", "DRR deficit round robin");
#endif
  printf(
      "===================================================================\n");
}

void app_init(void) {
  app_init_params();
  app_init_mbuf_pools();
  app_init_rings();
  app_init_ports();
  app_print_switchinfo();
  RTE_LOG(INFO, USER1, "Initialization completed\n");
}
