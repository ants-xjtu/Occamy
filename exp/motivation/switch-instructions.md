This document provides a comprehensive guide to configure the Explicit Congestion Notification (ECN) feature. Below is an example for configuring ECN on `port0`:

#### Configuration Steps:
1. Enter system view:
   ```bash
   system-view
   ```

2. Create and configure a drop profile:
   ```bash
   drop-profile ecn-750k
   ```
   - This creates a drop profile named `ecn-750k`.
   - Enter the drop profile view and set ECN thresholds to 10KB:
     ```bash
     ecn buffer-size low-limit 10240 high-limit 10240 discard-percentage 100
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
 qos queue 0 wred ecn-10k
 qos queue 0 ecn
 device transceiver 10GBASE-FIBER
 port mode 10G
#
return
```

---

### Buffer Size Adjustment

The buffer size can only be adjusted by modifying the **loss-less pool** ratio. The current loss-less pool ratio is 92%, leaving approximately 2MB of buffer space for lossy traffic.

You can estimate the available lossy buffer size by checking the current buffer usage:
```bash
display qos buffer usage
```

---

### DT's Alpha Parameter

In Huawei switches, the **Dynamic value** parameter corresponds to the `alpha` value as follows:

You can set alpha to 8 by changing the dynamic value to 10, set alpha to 1 by changing the dynamic value to 6.
