# Programmable Priority Encoder (PPE) Library

This repository provides various implementations of **Programmable Priority Encoders (PPE)**, which are essential building blocks for **Round-Robin (RR) Scheduling** in high-performance networking and bus arbitration.

The library is divided into two main categories: **Hierarchical PPE** (for high-port-count switches) and **Classic Single-Queue PPE**.

---

## 1. Hierarchical PPE Implementations

These modules are designed for **Switch Fabric RR Scheduling**, specifically optimized for scenarios involving **multiple ports** and **multiple queues**.

* **`ppe_h_p.v` (Pipelined Hierarchical PPE)**
* **Description:** A hierarchical architecture that incorporates pipeline registers between the top-level and sub-level arbitration.
* **Use Case:** High-frequency designs (e.g., >200MHz) where timing closure is a priority. It trades off 1 cycle of latency for significantly improved .


* **`ppe_h_simple.v` (Single-Cycle Hierarchical PPE)**
* **Description:** A purely combinational hierarchical implementation.
* **Use Case:** Low-latency applications where arbitration results are required within a single clock cycle. Best suited for smaller port counts or lower clock frequencies.



---

## 2. Classic PPE Implementations

These are standard implementations for typical single-queue or single-stage arbitration needs.

* **`ppe_com.v` (Registered Single-Queue PPE)**
* **Description:** A complete, standalone PPE wrapper. It includes internal registers for the Request, Grant, and Priority Pointer (`P_enc`).
* **Key Feature:** Implements a closed-loop feedback system where the priority pointer is automatically updated (rotated) after every successful grant.



---

## 3. Core Logic Components (Dependencies)

The following sub-modules provide the underlying combinational logic used by the top-level designs:

| Module | Description |
| --- | --- |
| **`ppe.v`** | The core **Purely Combinational PPE**. Implements the mask-and-arbitrate logic using two parallel paths (Thermo and Simple) for single-cycle RR. |
| **`encoder.v`** | Converts a One-Hot Grant vector into a binary index for priority pointer updates. |
| **`smpl_pe.v`** | A standard **Simple Priority Encoder** (Fixed Priority) that serves as the base arbitration unit. |
| **`thermometer.v`** | Generates a **Thermometer Code Mask** based on the priority pointer to "shield" lower-priority requests. |
