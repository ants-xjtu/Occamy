# DPDK based software prototype experiment description and scripts.

## Overview

This document describes the experiments on dpdk-based software prototype in §6.2 and §6.3.

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

- Query traffic.

- Background traffic.

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
├── absorb_bursts         # Scripts for Figure 12.
├── alpha                 # Scripts for Figure 15.
├── buffer_chocking       # Scripts for Figure 14.
├── general               # general scripts for all experiments.
└── performance_isolation # Scripts for Figure 13.
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

1. A host with this repository, this repository contains dpdk switch and scripts for running experiments. 

> how to run dpdk-switch is explained in detail in this [documentation](../../src/dpdk/README.md).

```bash
git clone https://github.com/ants-xjtu/Occamy.git
```

- The dpdk switch need some special configuration which is explained in the [documentation](../../src/dpdk/README.md).

- The dpdk-switch is in `Occamy/src/dpdk/`.

- Dpdk-switch and subsequent experimental scripts run on the same machine.

- Dpdk-switch will be automatically run in the subsequent `run.sh` script.

2. 8 hosts witch [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) in the same path.

```bash
# in each host's home directory
git clone https://github.com/Hijack8/TrafficGenerator.git
```

3. The host with dpdk swtich can SSH login to 8 hosts as the root user.


4. Change the config file in `Occamy/src/dpdk/mac.txt` for dpdk-switch.

The `mac.txt` file contains the mac addresses of the 8 hosts' NIC ports connected to the switch, which facilitates the establishment of a static routing table.

5. Change the config file in `Occamy/exp/dpdk/general/config.conf` for experimental scripts.

The `config.conf`:
```bash
# 8 hosts' control ip for ssh connect : ssh root@IP
SERVER_LIST=( "192.168.1.2" "192.168.1.3" "192.168.1.4" "192.168.1.5" "192.168.1.6" "192.168.1.7" "192.168.1.8" "192.168.1.9")
# local ip 
SWITCH_IP=( 127.0.0.1)
# dpdk switch path 
dpdk_switch_path="/path/to/dpdk-switch/"
# 8 hosts' same TrafficGenerator path. if it is in your home directory, it will be /home/username/TrafficGenerator/ 
traffic_path="/path/to/TrafficGenerator/" #example: "/home/xxx/TrafficGenerator/"
# 8 hosts' nic port IP connected to the swtich. 
SERVER_EXP_IPS=( "192.168.10.2" "192.168.10.3" "192.168.10.4" "192.168.10.5" "192.168.10.6" "192.168.10.7" "192.168.10.8" "192.168.10.9")
# MAC addresses of all SERVER_EXP_IPS
SERVER_MACS=( "00:00:00:00:00:00" "00:00:00:00:00:01" "00:00:00:00:00:02" "00:00:00:00:00:03" "00:00:00:00:00:04" "00:00:00:00:00:05" "00:00:00:00:00:06" "00:00:00:00:00:07")
# NIC names of all SERVER_EXP_IPS
SERVER_NICS=( "enp4s0f0" "enp4s0f0" "enp4s0f0" "enp4s0f0" "enp4s0f0" "enp4s0f0" "enp4s0f0" "enp4s0f0")
# one of the SERVER_LIST 
ONE_CLIENT_IP="192.168.1.2"
```

## Reproduce Figure 12

### Run the scripts
```bash
cd absorb_bursts
sudo su
./run.sh
```

### Draw the figure

```bash
mkdir -p figure
python3 get_result.py
```

The figures will appear under `figure/`

- Figure 12(a)(b): `figure/query.png`
- Figure 12(c): `figure/background.png`
- Figure 12(d): `figure/background_small.png`


## Reproduce Figure 13

### Run the scripts
```bash
cd performance_isolation
sudo su
./run.sh
```

### Draw the figure

```bash
mkdir -p figure
python3 get_result.py
```
The figures will appear under `figure/`
- Figure 13(a)(b): `figure/query.png`



## Reproduce Figure 14

### Run the scripts
```bash
cd buffer_chocking
sudo su
./run.sh
```

### Draw the figure

```bash
mkdir -p figure
python3 get_result.py
```
The figures will appear under `figure/`
- Figure 14(a)(b): `figure/query.png`


## Reproduce Figure 15

### Run the scripts
```bash
cd alpha
sudo su
./run.sh
```

### Draw the figure

```bash
mkdir -p figure
python3 get_result.py
```
The figures will appear under `figure/`
- Figure 15(a): `figure/query_dt.png`
- Figure 15(b): `figure/query_occamy.png`
