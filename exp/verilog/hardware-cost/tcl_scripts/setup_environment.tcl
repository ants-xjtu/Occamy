# setup_environment.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 2: Set Design Compiler options
stage_message 2 "Setting Design Compiler options" 1

# Retrieve environment variables
set SYN_ROOT_PATH   [getenv "SYN_ROOT_PATH"]
set WORK_PATH       [getenv "WORK_PATH"]
set DC_PATH         [getenv "DC_PATH"]
set RTL_PATH        [getenv "RTL_PATH"]
set CONFIG_PATH     [getenv "CONFIG_PATH"]
set SCRIPT_PATH     [getenv "SCRIPT_PATH"]
set MAPPED_PATH     [getenv "MAPPED_PATH"]
set REPORT_PATH     [getenv "REPORT_PATH"]
set UNMAPPED_PATH   [getenv "UNMAPPED_PATH"]
set LIB_PATH        [getenv "LIB_PATH"]
set TOP_MODULE      [getenv "TOP_MODULE"]
set_host_options -max_cores 4

# Output variable values for verification
puts "SYN_ROOT_PATH: $SYN_ROOT_PATH"
puts "WORK_PATH:     $WORK_PATH"
puts "TOP_MODULE:    $TOP_MODULE"

# Set up Design Compiler search paths and libraries
set_app_var search_path [list . $RTL_PATH $LIB_PATH ${DC_PATH}/libraries/syn]
set_app_var target_library  [list gscl45nm.db]
set_app_var link_library    [list * gscl45nm.db]

stage_message 2 "Design Compiler options set" 0
