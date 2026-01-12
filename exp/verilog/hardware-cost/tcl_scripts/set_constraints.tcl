# set_constraints.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 5: Set timing and area constraints
stage_message 5 "Setting timing and area constraints" 1

# Create clock constraint
create_clock -period 0.2 -name clk [get_ports clk]

# Set delays for RAM modules
set_blackbox_delays "dpsram_*" 2 1

# Set delays for FIFO modules
set_blackbox_delays "sfifo_*" 2 1

set_switching_activity -toggle_rate 1.0 [get_ports clk]      # clock
set_switching_activity -toggle_rate 0.1 -static_probability 0.5 [all_inputs]
set_wire_load_mode top

# Set maximum area (0 means no limit)
set_max_area 0

stage_message 5 "Timing and area constraints set" 0
