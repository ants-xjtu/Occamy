# read_design.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 3: Read design files
stage_message 3 "Reading design files" 1

# Get all Verilog files under the RTL directory
set rtl_files [get_verilog_files $RTL_PATH]
# set ppe_h_width [getenv "PPE_H_WIDTH"]
# set ppe_h_log_w [getenv "PPE_H_LOG_W"]
# set ppe_h_width_T [getenv "PPE_H_WIDTH_T"]
# set ppe_h_log_w_T [getenv "PPE_H_LOG_W_T"]
set elab_params [getenv "ELAB_PARAMS"]



# Read all Verilog files
foreach file $rtl_files {
    puts "Reading file: $file"
    set last_slash [string last "/" $file]
    set module_name [string range $file [expr $last_slash + 1] end]
    set module_name [string range $module_name 0 end-2]
    puts $last_slash
    analyze -f verilog $file
    # elaborate $module_name
    puts $file
    puts $module_name
}

if {$elab_params eq ""} {
    puts "INFO: No elaboration parameters provided"
    exit 
    elaborate $TOP_MODULE
} else {
    puts "INFO: Elaborate with parameters: $elab_params"
    elaborate $TOP_MODULE -parameter $elab_params
}

link

stage_message 3 "Design files read" 0
