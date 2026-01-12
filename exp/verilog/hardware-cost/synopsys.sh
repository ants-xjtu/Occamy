#!/bin/bash

# ==============================================================================
# Synopsys Design Compiler 自动化运行脚本
# ==============================================================================

# 配置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # 无颜色

function setup_env() {
    # 1. 自动检测 dc_shell 路径
    if [ -z "$DC_PATH" ]; then
        local dc_bin=$(which dc_shell 2>/dev/null)
        if [ -n "$dc_bin" ]; then
            export DC_PATH=$(dirname "$dc_bin")
        elif [ -n "$2" ]; then
            export DC_PATH="$2"
        else
            echo -e "${RED}ERROR: Design Compiler (dc_shell) not found in PATH or arguments.${NC}"
            exit 1
        fi
    fi

    # 2. 设置顶层模块名
    export TOP_MODULE="${1:-$(basename $(pwd))}"

    # 3. 定义并导出路径变量
    export SYN_ROOT_PATH=$(pwd)
    export RTL_PATH="$SYN_ROOT_PATH/rtl"
    export WORK_PATH="$SYN_ROOT_PATH/work"
    export SCRIPT_PATH="$SYN_ROOT_PATH/script"
    export MAPPED_PATH="$SYN_ROOT_PATH/mapped"
    export REPORT_PATH="$SYN_ROOT_PATH/report"
    export LIB_PATH="$SYN_ROOT_PATH/library"

    # 4. 自动化创建缺失的目录
    local dirs=("$WORK_PATH" "$MAPPED_PATH" "$REPORT_PATH" "$LIB_PATH")
    for dir in "${dirs[@]}"; do
        [ ! -d "$dir" ] && mkdir -p "$dir"
    done

    echo -e "${GREEN}Environment configured for module: $TOP_MODULE${NC}"
}

function run_synthesis() {
    setup_env "$1" "$2"

    # 处理传递给 TCL 的参数 (例如: PPE_C_WIDTH=$3 PPE_C_LOG_W=$4)
    # 如果有更多参数需求，可以根据位置变量 $3, $4... 继续扩展
    local width=${3:-8}
    local log_w=${4:-3}
    export ELAB_PARAMS="PPE_C_WIDTH=$width,PPE_C_LOG_W=$log_w"

    if [ ! -f "$SCRIPT_PATH/main.tcl" ]; then
        echo -e "${RED}ERROR: main.tcl not found in $SCRIPT_PATH${NC}"
        exit 1
    fi

    echo "------------------- Synthesis Starting -------------------"
    echo "Parameters: $ELAB_PARAMS"
    
    cd "$WORK_PATH" || exit 1
    # 运行 DC 并实时显示日志，同时备份到 execute.log
    dc_shell -f "$SCRIPT_PATH/main.tcl" | tee "$SYN_ROOT_PATH/execute.log"
    
    echo "------------------- Synthesis Finished -------------------"
    cd "$SYN_ROOT_PATH"
}

function clean_env() {
    echo -e "${RED}Cleaning environment...${NC}"
    setup_env "$@"
    
    # 清理日志和结果目录
    rm -f "$SYN_ROOT_PATH/execute.log"
    rm -rf "$REPORT_PATH"/*
    rm -rf "$MAPPED_PATH"/*
    
    # 清理 work 目录但保留隐藏的配置文件
    if [ -d "$WORK_PATH" ]; then
        find "$WORK_PATH" -type f ! -name ".*" -delete
    fi
    echo "Clean complete."
}

# -------------------------------------------- Main Logic --------------------------------------------

case "$1" in
    --run | -r)
        shift
        run_synthesis "$@"
        ;;
    --clean | -c)
        shift
        clean_env "$@"
        ;;
    --help | -h | *)
        echo "Usage: $0 {--run|--clean} [TopModule] [DCPath] [Width] [LogW]"
        echo "Example: $0 --run my_design /opt/synopsys/bin 16 4"
        ;;
esac