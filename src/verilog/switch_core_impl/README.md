# Occamy Switch Core v2

This project implements the **Occamy Switch Core**, a high-performance cell-based switching engine designed for multi-port networking applications. It features a separated data and control path architecture with integrated Round-Robin scheduling and congestion management.

## Core Architecture

The system utilizes a **Cell-Based Linked List** approach for memory management:

* 
**Data Path**: 512-bit wide data bus for high-throughput cell processing.


* 
**Control Path**: Manages cell pointers and packet descriptors independently from the raw data.


* 
**Pointer Management**: A dedicated controller initializes a free-pointer chain () upon reset to track available memory slots.



## Key Modules

| Module | Description |
| --- | --- |
| **`admission.v`** | Handles packet ingress, cell segmentation, and buffer admission based on port bitmaps.

 |
| **`cell_pointer_memory_control.v`** | Manages the Free Queue (FQ) and linked-list RAM for cell pointers.

 |
| **`cell_read_v2.v`** | Implements the egress scheduler using PPE and DRR algorithms to fetch cells for transmission.

 |
| **`ppe_8_func.v`** | Provides 8-port Programmable Priority Encoding for fair Round-Robin arbitration.

 |
| **`headdrop_v5.v`** | Congestion control mechanism that drops packets at the head of the queue to free up buffer space.

 |
| **`statistics_v2.v`** | Tracks queue lengths (QLEN) and generates admission control bitmaps based on buffer occupancy.

 |

## Key Features

* 
**Programmable Scheduling**: Uses a mask-based PPE to ensure fair port arbitration by shifting priority after each grant.


* 
**Scalable Memory**: Supports dual-port SRAM for concurrent admission and read-out operations.


* 
**Traffic Management**: Implements dynamic threshold-based admission control to prevent buffer exhaustion.


* 
**DRR Support**: Includes Deficit Round Robin counters to manage output bandwidth per port.



## Hardware Requirements

* 
**Memory**: Requires Dual-Port SRAM (DPSRAM) for Data and Pointer storage.


* 
**Logic**: Optimized for 8-port configurations with configurable widths for Data and Pointers.