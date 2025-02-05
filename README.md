# Occamy
Occamy is a preemptive buffer management for on-chip shared-memory switches.
It can quickly vacate the over-allocated buffer with head drop.

This repo contains the source code to reproduce results in the EuroSys'25 paper.


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

## Reproducing Results
The details for reproducing the experiment results are provided in a separate README file.

- Motivation
    - [DT's anomalous behavior on commodity switching chips](https://github.com/ants-xjtu/Occamy/tree/eurosys25-artifacts/exp/motivation)
    - [DT's buffer/memory bandwidth utilization](https://github.com/ants-xjtu/Occamy-Simulation)
- [P4-based experiments](https://github.com/ants-xjtu/Occamy/tree/eurosys25-artifacts/exp/p4)
- [DPDK-based experiments](https://github.com/ants-xjtu/Occamy/tree/eurosys25-artifacts/exp/dpdk)
- [Verilog-based experiments](https://github.com/ants-xjtu/Occamy/tree/eurosys25-artifacts/exp/verilog)
- [Simulations](https://github.com/ants-xjtu/Occamy-Simulation)

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

