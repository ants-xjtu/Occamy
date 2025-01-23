#!/bin/bash 

dir=$(git rev-parse --show-toplevel)
cd $SDE/build
$SDE/run_bfshell.sh -b init.py
