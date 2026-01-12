# check_design.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 4: Check link and design integrity
stage_message 4 "Checking link and design integrity" 1

if {[link] != 1} {
    puts "ERROR: Link failed!"
    exit 1
}
if {[check_design] != 1} {
    puts "ERROR: Design check failed!"
    exit 1
}

stage_message 4 "Link and design check completed" 0
