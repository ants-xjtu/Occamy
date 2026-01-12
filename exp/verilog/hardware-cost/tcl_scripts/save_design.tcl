# save_design.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 8: Save the synthesized design
stage_message 8 "Saving synthesized design" 1

# Create mapped directory if it doesn't exist
file mkdir "$MAPPED_PATH"

# Save the synthesized netlist (Verilog format)
write -format verilog -hierarchy -output "$MAPPED_PATH/${TOP_MODULE}_synthesized.v"

write -hierarchy -mapped -output "$MAPPED_PATH/${TOP_MODULE}_mapped_netlist.v"

# Save the synthesized design (DDC format)
write -format ddc -hierarchy -output "$MAPPED_PATH/${TOP_MODULE}.ddc"
# Save the constraint file (SDC format)
write_sdc "$MAPPED_PATH/${TOP_MODULE}.sdc"

stage_message 8 "Synthesized design saved" 0
