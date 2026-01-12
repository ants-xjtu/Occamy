# main.tcl
# gui_start
# Disable command echoing and verbose output
set echo off
set verbose off

# Source common functions
set SCRIPT_PATH [getenv "SCRIPT_PATH"]
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 1: Clean previous design data
stage_message 1 "Cleaning design environment (remove_design -all)" 1
remove_design -all
stage_message 1 "Design environment cleaned" 0

# Source individual stage scripts
source "$SCRIPT_PATH/setup_environment.tcl"
source "$SCRIPT_PATH/read_design.tcl"
source "$SCRIPT_PATH/check_design.tcl"
source "$SCRIPT_PATH/set_constraints.tcl"
source "$SCRIPT_PATH/synthesis.tcl"
source "$SCRIPT_PATH/generate_reports.tcl"
source "$SCRIPT_PATH/save_design.tcl"

# Stage 9: Exit Design Compiler
stage_message 9 "Exiting Design Compiler" 1

exit
