# Occamy

> **📦 EuroSys'25 Artifact Evaluation:**
> The artifacts for our EuroSys'25 paper are available on the [eurosys25-artifacts](https://github.com/ants-xjtu/Occamy/tree/eurosys25-artifacts) branch.
> Please switch to that branch for detailed instructions on reproducing the experimental results.

Occamy is a preemptive buffer management for on-chip shared-memory switches.
It can quickly vacate the over-allocated buffer with head drop.

## Code Structure
- `src`: The source code of prototypes, including
    - `src/dpdk`: DPDK-based software prototype
    - `src/p4`: P4-based hardware prototype
    - `src/verilog`: Verilog implementation of core components
- `exp`: The scripts to reproduce experiment results, including
    - `exp/motivation`: DT's anomalous behavior on commodity switching chips
    - `exp/dpdk`: DPDK-based experiments
    - `exp/p4`: P4-based experiments
    - `exp/verilog`: Verilog-based experiments

The ns-3 code has been relocated to a separate [repository](https://github.com/ants-xjtu/Occamy-Simulation)
due to its large overall size.

## Reference
The design, implementation, and evaluation of Occamy are detailed in the following paper:
```bib
@inproceedings{Occamy,
    author = {Shan, Danfeng and Li, Yunguang and Ma, Jinchao and Zhang, Zhenxing and Liang, Zeyu and Wen, Xinyu and Li, Hao and Jiang, Wanchun and Li, Nan and Ren, Fengyuan},
    title = {Occamy: A Preemptive Buffer Management for On-chip Shared-memory Switches},
    year = {2025},
    booktitle = {EuroSys},
}
```

