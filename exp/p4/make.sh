#!/bin/bash 

dir=$(git rev-parse --show-toplevel)
cd $SDE/p4studio
flg="-D HEAD_DROP"

if [ $2 == "--dt" ]; then
    flg=""
fi

if [ $1 ]; then
    prgrm=alpha_$1
    cmake \
        -DCMAKE_INSTALL_PREFIX=$SDE_INSTALL \
        -DCMAKE_MODULE_PATH=$SDE/cmake \
        -DP4_NAME=occamy \
        -DP4_PATH=${dir}/src/p4/${prgrm}.p4 \
        -DP4FLAGS="--create-graphs -g" \
        -DP4PPFLAGS="${flg}"
else
    echo "Specify an alpha of 1, 2 or 4."
fi

make occamy && make install
