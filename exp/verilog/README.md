# Verilog report 

This directory automates the synthesis of Verilog designs using Synopsys Design Compiler and generates timing, area reports. The setup includes a configuration file, a shell script to run the process, and a TCL script that drives the synthesis flow.

## Files Overview

- **`config.conf`**: Configuration file containing paths for the Design Compiler, Verilog design files, library files, and output report directories.
  
- **`run.sh`**: Shell script that loads the configuration, starts the Design Compiler, and triggers the synthesis process. It accepts a top-level module name as a command-line argument.

- **`main.tcl`**: TCL script executed by Design Compiler.
  
## Setup

1. **Configure paths**: Write a `config.conf` file based on `config.conf.example`.

2. **Run the script**: Execute the `run.sh` script, providing the top-level module name as an argument:
   
   ```bash
   ./run.sh top_module_name
   ```

## Reports

- **Timing Report**: Generated as `timing_report_<TOP_MODULE>.txt`
- **Area Report**: Generated as `area_report_<TOP_MODULE>.txt`

All reports are saved to the directory specified by `REPORT_DIR`.

## Requirements

- **Synopsys Design Compiler** 2016
- Vivado 2024
