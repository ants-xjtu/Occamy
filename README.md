# Occamy
Occamy is a preemptive buffer management for on-chip shared-memory switches.
It can quickly vacate the over-allocated buffer with head drop.

This repo contains the source code to reproduce results in the EuroSys'25 paper.


## Code Structure
- `src`: The source code of prototypes, including DPDK-based software prototype, P4-based hardware prototype, and verilog implementation of core components.
- `exp`: The scripts to reproduce experiment results.

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

