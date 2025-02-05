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


## Our configuration

- We build a testbed comprising 4 hosts connected to a Huawei CE6865 switch.

- Each host is equipped with an Intel XL710 Dual Port 40GbE NIC.

- Using network namespaces, we isolated two ports on each NIC to emulate two separate NICs and hosts.

[Here](./switch-instructions.md) is how to use the HUAWEI switch.

- Using [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) to generate query burst traffic.

- Using [iperf3](https://github.com/esnet/iperf.git) to generate background traffic.


## Reproduce Figure 5

### Set up configuration file

```bash
cd motivation
mv config.conf.example config.conf
# change the config.conf file.
```

### Configure the Huawei switch following [switch-instructions](switch-instructions.md)

- Turn on ECN
- Enable multiple queues, and distinguish each queue based on the DSCP value
- Set the alpha of queue 0 to 8 and the alpha of other queues to 1

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