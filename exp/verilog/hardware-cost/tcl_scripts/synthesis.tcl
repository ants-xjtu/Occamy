# synthesis.tcl

# Source common functions
source "$SCRIPT_PATH/common_functions.tcl"

# Stage 6: Perform synthesis
stage_message 6 "Starting synthesis" 1

# Compile the design with high effort
# compile -map_effort high -area_effort high
compile_ultra

stage_message 6 "Synthesis completed" 0
