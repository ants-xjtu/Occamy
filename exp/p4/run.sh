#!/bin/bash 

dir=$(git rev-parse --show-toplevel)
($dir/exp/p4/make.sh $1 $2) && $SDE/run_switchd.sh -p occamy
