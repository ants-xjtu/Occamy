#!/bin/bash

source config.conf
# Create a report directory if it doesn't exist
mkdir -p $REPORT_DIR

export TOP_MODULE=$1

# Start Design Compiler and read the script
$DC_PATH/bin/dc_shell-xg-t -f main.tcl <<EOF


