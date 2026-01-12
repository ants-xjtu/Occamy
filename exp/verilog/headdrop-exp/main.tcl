# Print configuration to verify

set DC_PATH [getenv "DC_PATH"]
set DESIGN_DIR [getenv "DESIGN_DIR"]
set REPORT_DIR [getenv "REPORT_DIR"]
set target_library [getenv "target_library"]
set link_library [getenv "link_library"]

set TOP_MODULE [getenv "TOP_MODULE"]

puts "DC_PATH: $DC_PATH"
puts "DESIGN_DIR: $DESIGN_DIR"
puts "REPORT_DIR: $REPORT_DIR"
puts "target_library: $target_library"
puts "link_library: $link_library"

# Read all Verilog files
set verilog_files [glob -nocomplain $DESIGN_DIR/*.v]
foreach file $verilog_files {
    puts "Reading file: $file"
    read_file -format verilog $file
}

# Set the top-level design
current_design $TOP_MODULE

create_clock -period 10 -name clk [get_ports clk]

set_max_area 0

# Compile the design
compile -map_effort high -area_effort high

# Generate Timing Report
report_timing  > $REPORT_DIR/timing_report_$TOP_MODULE.txt

# Generate Area Report
report_area
# Output to file
report_area > $REPORT_DIR/area_report_$TOP_MODULE.txt

# Optionally, you can generate power report
report_power > $REPORT_DIR/power_report_$TOP_MODULE.txt

# Exit Design Compiler
exit

