# Motivation experiments based on HUAWEI switch 

## testbed configuration 

- A switch that supports ecn mark, dt buffer management, and adjusts dt's alpha.

- 8 hosts with 40GbE NICs.

## experiment configuration 

- The switch has 2MB fully shared buffer, dynamically allocated to eight 40Gbps ports via DT. 

- The switch supports 8 class-of-service queues, with one designated as a high-priority queue, while the remaining queues are low-priority. 

- We employ DCTCP as the congestion control algorithm, with the ECN threshold set to 300kB. 

### buffer chocing (Figure 5(a))

- We generate two types of traffic, which are from different senders to the same receiver: 

  - High-priority incast traffic: the receiver simultaneously sends queries to 5 senders, each responding with data upon receiving the query. The incast degree is 40 (i.e., 8 flows per sender). 

  - Low-priority background traffic: We generate 14 long-lived flows from 2 other senders and to the same receiver, which are classified into 7 low-priority queues, respectively. 

- To examine the performance of incast traffic, we measure the query completion time (QCT), which is the completion time of all incast flows. 

- We compare QCT between two scenarios: one with low-priority traffic and the other without. For a fair comparison, we configure DT such that the incast traffic should be allocated with the same amount of buffer in both scenarios. 

  - Specifically, we set α = 8 for the high-priority queue with low-priority traffic and α = 1 without low-priority traffic. 

  - For low-priority queues, we set α = 1. In this way, the incast traffic deserves 1MB buffer either with or without low-priority traffic, and ideally the QCT performance should be unaffected by low-priority traffic.

### performance isolation (Figure 5(b))

- we use the same experimental settings as before, except that two types of traffic are congested at different ports, thereby eliminating the impact of buffer choking.


## A demo (Our configuration)

- We build a testbed comprising 4 hosts connected to a Huawei CE6865 switch. 

- Each host is equipped with an Intel XL710 Dual Port 40GbE NIC. 

- Using network namespaces, we isolated two ports on each NIC to emulate two separate NICs and hosts.

[Here]() is how to use the HUAWEI switch.

- Using [TrafficGenerator](https://github.com/Hijack8/TrafficGenerator.git) to generate query burst traffic.

- Using [iperf3](https://github.com/esnet/iperf.git) to generate background traffic.


First, 2 senders need to send background traffic to the receiver. 

Taking Huawei switches as an example, background traffic is divided into seven different queues based on the dscp value.

```sh
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6666 -t 0 -S 32 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6667 -t 0 -S 64 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6668 -t 0 -S 96 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6669 -t 0 -S 128 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6610 -t 0 -S 160 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6611 -t 0 -S 192 
ip netns exec ns2 iperf3 -C dctcp -c 192.168.xx.xx -p 6612 -t 0 -S 224 
```

Second, use TrafficGenerator to generate incast traffic and record its qct.

```sh 
### IN sender 
cd TrafficGenerator; ./bin/server -p 6001 -d;

### IN receiver
cd TrafficGenerator; ./bin/client -c conf/xxx -b 1000 -t 120
```

> A sample TrafficGenerator configuration file
> ```sh 
>server 192.168.40.31 6001 1
>server 192.168.40.41 6001 1
>server 192.168.40.50 6001 1
>server 192.168.40.21 6001 1
>server 192.168.40.51 6001 1
>req_size_dist /home/ygli/TrafficGenerator/conf/INCAST_CDF.txt
>dscp 0 100
>rate 0Mbps 100
>fanout 40 100 
> ```


