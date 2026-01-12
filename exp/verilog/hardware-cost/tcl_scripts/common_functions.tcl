# common_functions.tcl

# Disable command echoing and verbose output
set echo off
set verbose off

# Procedure to output formatted stage messages
proc stage_message {stage_num stage_desc is_start} {
    set line "===================================================================================================================================================================================================================================================================================================================="
    if {$is_start} {
        puts "\n$line"
        puts "                          Stage $stage_num Start: $stage_desc"
        puts "$line\n"
    } else {
        puts "\n$line"
        puts "                          Stage $stage_num End: $stage_desc"
        puts "$line\n"
    }
}

# Procedure to recursively get Verilog files
proc get_verilog_files {dir} {
    set files [list]
    foreach item [glob -nocomplain -directory $dir *] {
        if {[file isdirectory $item]} {
            set subfiles [get_verilog_files $item]
            set files [concat $files $subfiles]
        } elseif {[string tolower [file extension $item]] == ".v"} {
            lappend files $item
        }
    }
    return $files
}

# Procedure to set arc delays for black box modules
proc set_blackbox_delays {cell_pattern read_delay write_delay} {
    set cells [get_cells -hierarchical -filter "ref_name =~ \"$cell_pattern\""]
    foreach_in_collection cell $cells {
        # Set read delay
        set_arc_delay -from [all_inputs -hierarchical -pinnames] \
                      -to [all_outputs -hierarchical -pinnames] \
                      -delay $read_delay $cell
        # Set write delay
        set_arc_delay -from [all_inputs -hierarchical -pinnames] \
                      -to [all_outputs -hierarchical -pinnames] \
                      -delay $write_delay $cell
    }
}
