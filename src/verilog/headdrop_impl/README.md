# verilog 

This directory contains Verilog modules for managing packet flow in a network device, including head-drop mechanisms and scheduling logic.

## Modules Overview

- **`headdrop_drop`**: Implements head-drop logic based on scheduler decisions, controlling packet memory (FQ and PD).
- **`headdrop_scheduler`**: Schedules packet transmission by selecting ports with queue lengths exceeding a threshold.
- **`ppe_64_new`**: Processes port selection with priority encoding based on queue lengths.
- **`ppe_8`**: An 8-port priority encoder with enabling and shifting logic.
- **`priority_encoder_8to3`**: 8-bit to 3-bit priority encoder.
- **`priority_encoder_4to2`**: 4-bit to 2-bit priority encoder.
- **`tothermo8`**: Converts a 3-bit input to an 8-bit output using thermal logic.
- `fixed_arb`: Fixed priority arbiter.
