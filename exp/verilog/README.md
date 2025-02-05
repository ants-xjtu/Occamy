# Verilog report 

This directory automates the synthesis of Verilog designs using Synopsys Design Compiler and generates timing, area reports. The setup includes a configuration file, a shell script to run the process, and a TCL script that drives the synthesis flow.

## Files Overview


- **`config.conf`**: Configuration file containing paths for the Design Compiler, Verilog design files, library files, and output report directories.
  
- **`run.sh`**: Shell script that loads the configuration, starts the Design Compiler, and triggers the synthesis process. It accepts a top-level module name as a command-line argument.

- **`main.tcl`**: TCL script executed by Design Compiler.

**These three files are the running scripts of asic cost, FPGA cost only needs graphical operation.**

## Requirements

- **Synopsys Design Compiler** 2016  

  - OS for Design Compiler: Ubuntu22.04

- Vivado 2024

  - OS for vivado: Windows11
 
## Reproduce Table 1.

### ASIC Cost

1. **Configure paths**: Write a `config.conf` file based on `config.conf.example`.

2. **Run the script**: Execute the `run.sh` script, providing the top-level module name as an argument:
 
   ```bash
   ./run.sh headdrop_scheduler
   ./run.sh fixed_arb
   ./run.sh headdrop_drop
   ```
3. **Report**: Report is in the `REPORT_DIR` path in config file.

- **Timing Report**: Generated as `timing_report_<TOP_MODULE>.txt`
- **Area Report**: Generated as `area_report_<TOP_MODULE>.txt`


### FPGA Cost


- Step 1: Launch Vivado. Open **Vivado 2024** from your Windows desktop.

- Step 2: Create or Open a Project. Set the FPGA card to [U50](https://www.amd.com/en/products/accelerators/alveo/u50/a-u50-p00g-pq-g.html).

    - select "Create New Project":
    - Enter the Project Name and select the Project Location.
    - Choose RTL Project.
    - Optionally, check Do not specify sources at this time if you plan to add them later.
    - Click Next and specify the Part or Board to [U50](https://www.amd.com/en/products/accelerators/alveo/u50/a-u50-p00g-pq-g.html).

- Step 3: Import Files (Sources) 

    1. **Add Source Files**:
        - Navigate to the **"Project Manager"** window (on the left side of the screen).
        - Right-click on **Sources** and select **Add Sources**.
        - Select the **type of source** you want to add (e.g., Verilog, VHDL, or XDC for constraints).
        - Browse to the location of your source files and select them.
        - Click **Finish**.

    2. **Add Constraints** (Optional, but often necessary):
        - Right-click on **Constraints** in the **Project Manager** and select **Add Constraints**.
        - Browse and select any `.xdc` or other constraint files (for pin assignments, timing constraints, etc.).
        - Click **Finish**.

- Step 4: Set a module as the top module. Under Sources, right-click on the module you want to set as the top and select Set as Top.

  In this experiment, top module should be `headdrop_scheduler`, `fixed_arb` or `headdrop_drop`.

- Step 5: Under the **Flow Navigator**, click **Run Synthesis**. 

- Step 6: Run Implementation.

- Step 7: Generate Utilization Report.

- Step 8: View and Save the Utilization Report.

    1. Once implementation is complete, go to Flow Navigator and click Open Implemented Design.
    2. Under the Reports section, click Utilization to generate the utilization report.
        - The report will display information about resource usage, including LUTs, flip-flops, block RAMs, DSPs, and other FPGA-specific resources.
        - You can also access Reports > Utilization from the main Vivado menu if needed.


Set top module to `headdrop_scheduler`, `fixed_arb` or `headdrop_drop` in Step 4 and repeat Step 5~8 to get the 3 modules' reports.
