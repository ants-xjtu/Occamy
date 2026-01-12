# Occamy Switch DC Synthesis Flow

This repository contains a modular and automated synthesis environment for **Synopsys Design Compiler (DC)**. It is designed to perform synthesis on the Occamy Switch components (like PPE and Hierarchical PPE) with support for parameterized elaboration.

---

## 1. Directory Structure

To use this flow, organize your project as follows:

* **`rtl/`**: Place all Verilog source files here.
* **`script/`**: Contains all `.tcl` files provided in this repository.
* **`library/`**: Place your technology library files (e.g., `gscl45nm.db`) here.
* **`work/`**: Temporary working directory for DC operations.
* **`report/`**: Output directory for timing, area, and power reports.
* **`mapped/`**: Output directory for synthesized netlists and SDC constraints.

---

## 2. Key Components

### Automation Script

* **`synopsys.sh`**: The main entry point. This Bash script handles environment setup, directory creation, parameter passing, and log management.



### TCL Flow

* 
**`main.tcl`**: The top-level script that orchestrates the synthesis stages.


* 
**`read_design.tcl`**: Automatically analyzes all RTL files and performs parameterized elaboration.


* 
**`set_constraints.tcl`**: Defines the timing environment, including a default clock period of **0.2ns**.


* 
**`synthesis.tcl`**: Executes the synthesis engine using `compile_ultra` for high-performance optimization.


* 
**`common_functions.tcl`**: Helper procedures for recursive file searching and modeling delays for black-box modules (RAM/FIFO).



---

## 3. Usage Instructions

### Run Synthesis

Execute the synthesis flow by passing the top module name and design parameters (e.g., width and log width):

```bash
# Usage: ./synopsys.sh --run [TopModule] [DCPath] [Width] [LogW]
./synopsys.sh --run ppe_h_p /path/to/synopsys/bin 16 4

```

* 
**`Width`**: Maps to `PPE_C_WIDTH`.


* 
**`LogW`**: Maps to `PPE_C_LOG_W`.



### Clean Environment

To remove previous logs, reports, and working files:

```bash
./synopsys.sh --clean

```

---

## 4. Synthesis Features

* 
**Parameterized Support**: Automatically passes `ELAB_PARAMS` from the shell to the DC `elaborate` command.


* 
**Black-Box Modeling**: Automatically sets arc delays for `dpsram_*` and `sfifo_*` modules to ensure realistic timing analysis for memory components.


* 
**Comprehensive Reporting**: Generates timestamped reports for timing, area, power, and clocking, labeled with the specific parameters used during the run.


* 
**Multi-Core Support**: Configured to utilize up to 4 CPU cores for faster compilation.