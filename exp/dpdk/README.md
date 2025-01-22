# DPDK based software prototype experiment description and scripts.

## Overview

This document describes the dpdk based software prototype experiment in §6.1, §6.2, §6.3. 

We provide detailed parameters and scripts to implement this experiment.

---

## Testbed Setup

### Switch Emulation
- **Server Specifications**:
  - Server with two Intel XL710 Quad Port 10GbE NICs.
  - Emulates a software switch with DPDK.
- **Buffer Configuration**:
  - Following Broadcom Tomahawk switch chip specifications:
    - **Buffer-per-port-per-Gbps**: 5.12kB.
    - **Total Buffer**: 410kB (5.12kB × 8 ports × 10Gbps).

### Host Configuration
- **Number of Hosts**: 8.
- **Host Specifications**:
  - Dell PowerEdge R730 server.
  - **CPU**: 16-core Intel Xeon E5-2620 2.1GHz.
  - **Memory**: 16GB DDR4.
  - **NIC**: Intel 82599 10GbE NIC.
  - Connected to the DPDK-based software switch via 10Gbps links.

### Traffic Generator 

We use [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) to generate 2 kinds of traffic.

- query traffic.

- background traffic.

---

## Congestion Control Algorithm

- **Algorithm**: DCTCP (Data Center TCP) with ECN (Explicit Congestion Notification).
- **ECN Threshold**: 65 packets (as suggested by [5]).


## Current experimental configuration

### Host IP Configuration
1. **Host Machines**:
   - **Number of Hosts**: 8.
   - **IP Addresses**: The IP addresses assigned to the host machines are in the range `192.168.1.2` to `192.168.1.9`.

2. **Switch-facing NICs on Hosts**:
   - Each host connects to the switch through a dedicated network interface card (NIC).
   - **IP Addresses for NICs**: The IP addresses assigned to these NICs are in the range `192.168.10.2` to `192.168.10.9`.

3. The other host which is running the DPDK-based switch run the scripts to control these 8 hosts.

| **Host** | **Host IP (192.168.1.x)** | **Switch-facing NIC IP (192.168.10.x)** |
|----------|----------------------------|-----------------------------------------|
| Host 1   | 192.168.1.2               | 192.168.10.2                            |
| Host 2   | 192.168.1.3               | 192.168.10.3                            |
| Host 3   | 192.168.1.4               | 192.168.10.4                            |
| Host 4   | 192.168.1.5               | 192.168.10.5                            |
| Host 5   | 192.168.1.6               | 192.168.10.6                            |
| Host 6   | 192.168.1.7               | 192.168.10.7                            |
| Host 7   | 192.168.1.8               | 192.168.10.8                            |
| Host 8   | 192.168.1.9               | 192.168.10.9                            |

---

### Traffic Generator Application
- Each host runs a **TrafficGenerator** application.
- **Path to TrafficGenerator**:
  - The TrafficGenerator application is located at the same file path on all hosts, ensuring uniform configuration and management.


### Scripts structrue


```
.
├── absorb_bursts         # Scripts for figure10.
├── alpha                 # Scripts for figure13.
├── buffer_chocking       # Scripts for figure12.
├── general               # general scripts for all experiments.
└── performance_isolation # Scripts for figure11.
```

### Parameters in scripts

```sh
# methods of BM
method_list=("DT" "Occamy" "ABM" "Pushout")
# alpha of each BM 
alpha_list=(0 3 1 0)
# proportion of base_size of each BM
query_size_list=(0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4)
# TrafficGenerator generating time(time of query traffic is equal to time of the background traffic)(/second)
exp_time=180
# The basic query size(/B)
base_query_size="419430"
# TrafficGenerator query traffic load(/Mbps)
exp_query_load=100
# TrafficGenerator background traffic load(/Mbps)
exp_bg_load=5000
# congestion_control(0->cubic 1->reno 2->dctcp)
congestion_control=2
# switch scheduling algorithm (0->drr 1->pq) use dscp to distinguish queues
switch_scheduling=0
# host scheduling algorithm (0->drr 1->pq) use dscp to distinguish queues
host_scheduling=0
# TrafficGenerator query traffic and background traffic are in the same queue? 0->yes 1->no
multi_queue=0
# query traffic queus
query_queue_num=1
# Individual congestion control algorithm for background traffic(In some cases, the query needs to be dctcp and the background needs to be cubic.) DEFAULT->same as query/DCTCP/CUBIC
exp_bak_cong="DEFAULT"
# background traffic queues
background_queue_num=1
# init cwnd (/pkts)
exp_init_cwnd=83

```

--- 

## Requirements

1. A host with dpdk-switch, how to run dpdk-switch is explained in detail in this [document](../../src/dpdk/README.md).

2. 8 hosts witch [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) in the same path.

3. The host with dpdk swtich can SSH login to 8 hosts as the root user.

## Reproduce Figure 10.

### run the scripts
```sh
cd absorb_bursts; 
sudo su; 
./run.sh
```

### plot 

```sh
mkdir -p figure
python3 get_result.py
```

the figure will appear under `figure/`

Figure10(a)(b) is in `figure/query.png`

Figure10(c) is `figure/background.png`

Figure10(d) is `figure/background_small.png`


## Reproduce Figure 11.

### run the scripts
```sh
cd performance_isolation; 
sudo su; 
./run.sh
```

### plot 

```sh
mkdir -p figure
python3 get_result.py
```
the figure will appear under `figure/`

Figure11(a)(b) is in `figure/query.png`



## Reproduce Figure 11.

### run the scripts
```sh
cd buffer_chocking; 
sudo su; 
./run.sh
```

### plot 

```sh
mkdir -p figure
python3 get_result.py
```
the figure will appear under `figure/`

Figure12(a)(b) is in `figure/query.png`


## Reproduce Figure 13.

### run the scripts
```sh
cd alpha; 
sudo su; 
./run.sh
```

### plot 

```sh
mkdir -p figure
python3 get_result.py
```
the figure will appear under `figure/`

Figure13(a) is `figure/query_dt.png`

Figure14(a) is `figure/query_occamy.png`



