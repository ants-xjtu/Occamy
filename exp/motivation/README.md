# Motivation experiments based on Huawei switch

## Testbed configuration

- A switch that supports ecn mark, dt buffer management, and adjusts dt's alpha.

- 8 hosts with 40GbE NICs.

## Experiment configuration

- The switch has 2MB fully shared buffer, dynamically allocated to eight 40Gbps ports via DT. 

- The switch supports 8 class-of-service queues, with one designated as a high-priority queue, while the remaining queues are low-priority.

- We employ DCTCP as the congestion control algorithm, with the ECN threshold set to 300kB. 

### Buffer choking (Figure 5(a))

- We generate two types of traffic, which are from different senders to the same receiver: 

  - High-priority incast traffic: the receiver simultaneously sends queries to 5 senders, each responding with data upon receiving the query. The incast degree is 40 (i.e., 8 flows per sender). 

  - Low-priority background traffic: We generate 14 long-lived flows from 2 other senders and to the same receiver, which are classified into 7 low-priority queues, respectively. 

- To examine the performance of incast traffic, we measure the query completion time (QCT), which is the completion time of all incast flows. 

- We compare QCT between two scenarios: one with low-priority traffic and the other without. For a fair comparison, we configure DT such that the incast traffic should be allocated with the same amount of buffer in both scenarios. 

  - Specifically, we set α = 8 for the high-priority queue with low-priority traffic and α = 1 without low-priority traffic. 

  - For low-priority queues, we set α = 1. In this way, the incast traffic deserves 1MB buffer either with or without low-priority traffic, and ideally the QCT performance should be unaffected by low-priority traffic.

### Performance isolation (Figure 5(b))

- We use the same experimental settings as before, except that two types of traffic are congested at different ports, thereby eliminating the impact of buffer choking.


---

## Our configuration

- We build a testbed comprising 4 hosts connected to a Huawei CE6865 switch.

- Each host is equipped with an Intel XL710 Dual Port 40GbE NIC.

- Using network namespaces, we isolated two ports on each NIC to emulate two separate NICs and hosts.

[Here](./switch-instructions.md) is how to use the HUAWEI switch.

- Using [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) to generate query burst traffic.

- Using [iperf3](https://github.com/esnet/iperf.git) to generate background traffic.

- We use a single host to control the other four hosts by ssh for streaming operations.

## Requirements

- Install TrafficGenerator iperf3 for 4 hosts.

```bash
apt install iperf3 
git clone https://github.com/Hijack8/TrafficGenerator.git
cd TrafficGenerator
make 
```
- Configure this repository on a separate host which can control other 4 hosts in root user by ssh.

```bash
git clone https://github.com/ants-xjtu/Occamy.git
```

## Reproduce Figure 5

### Set up configuration file

```bash
cd motivation
mv config.conf.example config.conf
# change the config.conf file.
```

```bash
# config.conf 
# 4 hosts' control ip for ssh connect : ssh root@IP
SERVER_LIST=( "192.168.1.12" "192.168.1.3" "192.168.1.4" "192.168.1.5")
# each host's path to TrafficGenerator
traffic_path="/path/to/TrafficGenerator/"
# host to recv query traffic 
ONE_CLIENT_IP="192.168.1.12"
# nic name of ONE_CLIENT_IP's 2 ports
ONE_CLIENT_NIC0="enp3s0f0np0"
ONE_CLIENT_NIC1="enp3s0f1np1"
```

### Configure the Huawei switch following [switch-instructions](switch-instructions.md)

**The following takes port0 as an example. In actual configuration, port0~port7 need to be set.**

- Turn on ECN, set the ecn threshold to 300KB.

```bash
system-view 

drop-profile ecn-300k
ecn buffer-size low-limit 307200 high-limit 307200 discard-percentage 100
quit

interface 100ge 1/0/1
qos queue 0 wred ecn-300k
qos queue 1 wred ecn-300k
qos queue 2 wred ecn-300k
qos queue 3 wred ecn-300k
qos queue 4 wred ecn-300k
qos queue 5 wred ecn-300k
qos queue 6 wred ecn-300k
qos queue 7 wred ecn-300k
qos queue 0 ecn
qos queue 1 ecn
qos queue 2 ecn
qos queue 3 ecn
qos queue 4 ecn
qos queue 5 ecn
qos queue 6 ecn
qos queue 7 ecn
commit
```

- Enable multiple queues, and distinguish each queue based on the DSCP value

```bash
system-view
trust dscp 
```

The correspondence between dscp value and queue number is explained in the [document](switch-instructions.md).


- Set the alpha of queue 0 to 8 and the alpha of other queues to 1

```bash
system-view
interface 100ge 1/0/1
qos buffer queue 0 shared-threshold dynamic 10
qos buffer queue 1 shared-threshold dynamic 6
qos buffer queue 2 shared-threshold dynamic 6
qos buffer queue 3 shared-threshold dynamic 6
qos buffer queue 4 shared-threshold dynamic 6
qos buffer queue 5 shared-threshold dynamic 6
qos buffer queue 6 shared-threshold dynamic 6
qos buffer queue 7 shared-threshold dynamic 6
commit
```

- Set the scheduling algorithm, queue0 is high priority, other queues are low priority and drr scheduled.

```bash
system-view 
qos drr 1 to 7
```

### Run the scripts

a. **The buffer chocking experiment**

```bash
cd buffer-chocking
sudo su
./run.sh
```

b. **The performance isolation experiment**

```bash
cd buffer-chocking
sudo su
./run.sh
```

c. **The experiment without background traffic (for comparison)**
```bash
cd buffer-chocking
sudo su
./run.sh
```

### Draw the figures

```sh
cd buffer-chocking
python3 get_result.py
```

The Figure 5(a) corresponds to `buffer-chocking/figure/query.png`.

```bash
cd performance_isolation
python3 get_result.py
```

The Figure 5(b) corresponds to `performance_isolation/figure/query.png`.
