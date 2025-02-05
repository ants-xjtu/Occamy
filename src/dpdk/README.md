# dpdk-switch

This is a l2 switch based on DPDK.

# Features

* Multiple queues per port with DRR/PQ queue scheduling

* Shared buffer with various buffer management schemes

* Fine-grained Queue length monitoring

* ECN marking

* High performance (70G ~ 80G)

# Requirements

* OS: Ubuntu22.04

* Hardware: 

  - 256G memory 
  - 32 cores or more.
  - 2x 4-port Intel X710 10GBE NIC.

* [DPDK](http://dpdk.org/) == 23.07

# Settings

* Boot settings: `crashkernel=auto rhgb quiet default_hugepagesz=1G hugepagesz=1G hugepages=8 isolcpus=0,2,4,6,8,10,12,14,16,18,20,22 intel_iommu=on iommu=pt`

* DPDK Settings: 32 pages of 1Gb, 64Gb in total

# How to Run it

1. Install requirements

2. Build and Setup DPDK library,
    [Build a APP](http://doc.dpdk.org/guides-23.07/linux_gsg/build_dpdk.html#building-app-using-installed-dpdk)

3. Create a static routing table for the switch

    ``$ cp mac.txt.example mac.txt``

4. run the script

    ```
    ./run.sh BM ALPHA_SHIFT SCHEDULER 

    BM: DT or Occamy or ABM or Pushout
    ALPHA_SHIFT: 3 -> (1 << 3 = 8)
    SCHEDULER: PQ or DRR

    ```
