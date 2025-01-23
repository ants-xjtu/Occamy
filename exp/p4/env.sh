#!/bin/bash 

export SDE=/root/bf-sde-9.9.0
export SDE_INSTALL=$SDE/install
export PATH=$SDE_INSTALL/bin:$PATH

# 加载 bf 驱动
if [ ! -e "/dev/bf0" ];then
    # echo "not exist!"
    echo "Load /dev/bf0"
    $SDE_INSTALL/bin/bf_kdrv_mod_load $SDE_INSTALL
fi
# echo "/dev/bf0 loaded."
# 检验加载驱动是否成功
# ls /dev/bf0

