### Configure multi-queue 

Using dscp to differentiate between different queues (taking the configuration of port 0 as an example).

```bash
# Enter the system view 
system-view 
# Enter the interface view
interface 25ge 1/0/1 
trust dscp
commit
```

By default, the correspondence between dscp values and queue indices is as shown in this [document](https://support.huawei.com/hedex/hdx.do?docid=EDOC1100317108&id=ZH-CN_CONCEPT_0141112308).

- dscp = 0~7    queue 0
- dscp = 8~15   queue 1 
- dscp = 16~23  queue 2 
- dscp = 24~31  queue 3 
- dscp = 32~39  queue 4 
- dscp = 40~47  queue 5 
- dscp = 48~55  queue 6 
- dscp = 56~63  queue 7


### Configure the scheduling algorithm

By default, the scheduling algorithm between queues is PQ(Priority Queue) scheduling. Queue0 has the highest priority and queue7 has the lowest priority.

If you want to set DRR scheduling, you can use the following steps (taking the configuration of port 0 as an example).


```bash
system-view 
interface 25ge 1/0/1 
qos drr 0 to 7
commit
```

### Configure ECN 

This document provides a comprehensive guide to configure the Explicit Congestion Notification (ECN) feature. Below is an example for configuring ECN on `port0`:

#### Configuration Steps:

1. Enter system view:
   ```bash
   system-view
   ```

2. Create and configure a drop profile (set the ecn threshold to 750kb)
   ```bash
   drop-profile ecn-750k
   ```
   - This creates a drop profile named `ecn-750k`.
   - Enter the drop profile view and set ECN thresholds to 750KB:
     ```bash
     ecn buffer-size low-limit 768000 high-limit 768000 discard-percentage 100
     ```
   - Exit the drop profile view:
     ```bash
     quit
     ```

3. Apply the configuration to the port:
   ```bash
   interface 25ge 1/0/1
   ```
   - Enter the port view for `25GE1/0/1`.
   - Apply the WRED drop template to the port queue:
     ```bash
     qos queue 0 wred ecn-750k
     ```
   - Enable the ECN feature for the specific queue:
     ```bash
     qos queue 0 ecn
     ```
   - Commit the configuration:
     ```bash
     commit
     ```

#### Verification:
Run the following command to verify the configuration:
```bash
display current-configuration interface 25ge 1/0/1
```

Expected output:

```plaintext
#
interface 25GE1/0/1
 qos queue 0 wred ecn-750k
 qos queue 0 ecn
 device transceiver 10GBASE-FIBER
 port mode 10G
#
return
```

---

### Buffer Size Adjustment

The buffer size can only be adjusted by modifying the **loss-less pool** ratio. The current loss-less pool ratio is 92%, leaving approximately 2MB of buffer space for lossy traffic.

```bash
system-view 
qos buffer lossless-pool percent 92 slot 1
commit
```

You can estimate the available lossy buffer size by checking the current buffer usage:
```bash
display qos buffer usage
```

---

### DT's Alpha Parameter

In Huawei switches, the **Dynamic value** parameter corresponds to the `alpha` value as follows:

You can set alpha to 8 by changing the dynamic value to 10, set alpha to 1 by changing the dynamic value to 6.

If you configure the alpha of queue 0 to be 8 and the alpha of queue 1 to be 1, you can do the following (taking port 0 as an example):

```bash
system-view
interface 25ge 1/0/1
 qos buffer queue 0 shared-threshold dynamic 10
 qos buffer queue 1 shared-threshold dynamic 6
 commit
```
