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

# Read all Verilog files from the DESIGN_DIR directory
set verilog_files [glob -nocomplain $DESIGN_DIR/*.v]
foreach file $verilog_files {
    analyze -f verilog $file
}

elaborate $TOP_MODULE

link

# Compile the design
compile_ultra

# Generate Timing Report
# -max_paths 10: Max number of paths in the report
# Output to file
report_timing  > $REPORT_DIR/timing_report_$TOP_MODULE.txt

# Generate Area Report
report_area
# Output to file
report_area > $REPORT_DIR/area_report_$TOP_MODULE.txt

# Optionally, you can generate power report
# report_power > $REPORT_DIR/power_report.txt

# Exit Design Compiler
exit

