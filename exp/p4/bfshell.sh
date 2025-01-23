#!/bin/bash 

dir=$(git rev-parse --show-toplevel)
cd $SDE/build

if [ $1 ]; then
    $SDE/run_bfshell.sh -b ${dir}/exp/p4/$1.py
else
    $SDE/run_bfshell.sh
fi
