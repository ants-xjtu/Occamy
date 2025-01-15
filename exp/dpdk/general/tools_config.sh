#!/bin/bash

del_tc_config() {
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}
    local hid=${hostip: -1}
    echo ${SERVER_NICS[$i]}
    ssh root@$hostip "tc qdisc del dev ${SERVER_NICS[$i]} root;" >/dev/null 2>&1
  done
}

sp_config() {
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}
    local hid=${hostip: -1}
    echo ${SERVER_NICS[$i]}
    ssh root@$hostip "tc qdisc del dev ${SERVER_NICS[$i]} root; tc qdisc add dev ${SERVER_NICS[$i]} root handle 1: prio" >/dev/null 2>&1
  done
}

change-query_size() {
  if [ $# -ne 2 ]; then
    echo "Usage: $0 [base value] [t]"
    echo "[value] can be in the format of '16K' or '1M'"
    exit 1
  fi

  local input=$1
  local unit=${input: -1} # 提取单位（最后一个字符）
  local value=${input%?}  # 提取数值部分
  case $unit in
  K | k)
    local m=$(($value * 1024))
    ;;
  M | m)
    local m=$(($value * 1024 * 1024))
    ;;
  *)
    if [[ $input =~ ^[0-9]+$ ]]; then
      local m=$input
    else
      echo "Invalid format. Please use 'K' for Kilobytes or 'M' for Megabytes."
      exit 1
    fi
    ;;

  esac
  # m=$((1024*1024))
  local k=$2
  local s=$(echo "scale=0; $m * $k" | bc)
  local ints=${s%.*}
  local ints1=$((ints + 1))
  local line1="$ints 0"
  local line2="$ints1 1"
  echo "Query size changed to $k * $input"
  for hostip in ${SERVER_LIST[@]}; do
    ssh root@$hostip "echo -e '$line1\n$line2' > ${traffic_path}conf/INCAST_CDF.txt"
  done
}

set-init-cwnd() {
  local expected_args_config=1

  usage() {
    echo "usage: $0 arg1"
    echo "arg1: init cwnd "
  }

  if [ $# -ne $expected_args_config ]; then
    usage
    exit 1
  fi
  local init_cwnd=$1
  echo "Set init-cwnd = $init_cwnd"
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}
    local hid=${hostip: -1}
    ssh root@$hostip "ip route change 192.168.10.0/24 dev ${SERVER_NICS[$i]} proto kernel scope link src 192.168.10.$hid initcwnd $init_cwnd"
  done
}

# arp-add() {
#   local hosts=("192.168.1.2" "192.168.1.3" "192.168.1.4" "192.168.1.5" "192.168.1.6" "192.168.1.8" "192.168.1.9" "192.168.1.10")
#   local hosts_of_mac=("192.168.3.12" "192.168.3.13" "192.168.3.14" "192.168.3.15" "192.168.3.16" "192.168.3.18" "192.168.3.19" "192.168.3.110")
#   local mac_addresses=("00:1b:21:bd:4a:b2" "00:1b:21:bd:4a:de" "64:9d:99:b2:2c:8d" "00:1b:21:c0:7c:0c" "90:e2:ba:8d:6f:19" "90:e2:ba:8d:52:c9" "90:e2:ba:8d:0a:a4" "90:e2:ba:8d:54:d0")
#
#   # 使用for循环按照下标遍历数组
#   for ((host_index = 0; host_index < ${#hosts[@]}; host_index++)); do
#     for ((mac_index = 0; mac_index < ${#mac_addresses[@]}; mac_index++)); do
#
#       if [[ $host_index -eq $mac_index ]]; then
#         continue
#       fi
#       local host="${hosts[$host_index]}"
#       local mac="${mac_addresses[$mac_index]}"
#       local host_of_mac="${hosts_of_mac[$mac_index]}"
#
#       ssh root@$host "arp -s $host_of_mac $mac"
#
#     done
#   done
# }

arp-add() {
  for ((host_index = 0; host_index < ${#SERVER_LIST[@]}; host_index++)); do
    for ((mac_index = 0; mac_index < ${#SERVER_MACS[@]}; mac_index++)); do

      if [[ $host_index -eq $mac_index ]]; then
        continue
      fi
      local host="${SERVER_LIST[$host_index]}"
      local mac="${SERVER_MACS[$mac_index]}"
      local host_of_mac="${SERVER_EXP_IPS[$mac_index]}"

      ssh root@$host "arp -s $host_of_mac $mac"

    done
  done
}

set-congestion-control() {
  local congestion_control

  case "$1" in
  0) congestion_control="cubic" ;;
  1) congestion_control="reno" ;;
  2) congestion_control="dctcp" ;;
  *)
    echo "Invalid input. Please use 0 for cubic, 1 for reno, or 2 for dctcp."
    exit 1
    ;;
  esac

  for hostip in ${SERVER_LIST[@]}; do
    ssh root@$hostip "sudo sysctl -w net.ipv4.tcp_congestion_control=$congestion_control" >/dev/null 2>&1

    # For dctcp, also set tcp_ecn
    if [ "$congestion_control" = "dctcp" ]; then
      ssh root@$hostip "sudo sysctl -w net.ipv4.tcp_ecn=1" >/dev/null
    fi
  done

  echo "Set TCP congestion control to $congestion_control"
}

configure() {
  local queue_num_1=$1
  local queue_num_2=$2
  local cubic_bg=$3
  local path="${traffic_path}conf/"
  local fanout=16

  for hostip in ${SERVER_LIST[@]}; do
    local bg_info=""
    local query_info=""

    for hip in ${SERVER_LIST[@]}; do
      local i="${hip: -1}"
      if [ "${hostip: -1}" == "$i" ]; then
        continue
      fi
      if [ "$cubic_bg" == "1" ]; then
        bg_info+="server 192.168.10.$i 5002 1\n"
      else
        bg_info+="server 192.168.10.$i 5001 1\n"
      fi
      query_info+="server 192.168.10.$i 5001 1\n"
    done

    query_info+="req_size_dist conf/INCAST_CDF.txt\n"
    bg_info+="req_size_dist conf/DCTCP_CDF.txt\n"

    bg_info+="$(calculate_dscp "$queue_num_2" "$queue_num_1")\n"
    query_info+="$(calculate_dscp "$queue_num_1")\n"

    bg_info+="rate 0Mbps 100"
    query_info+="rate 0Mbps 100\nfanout $fanout 100"

    ssh root@$hostip "echo -e '$bg_info' > ${path}dpdk_client_config.txt; echo -e '$query_info' > ${path}dpdk_incast_client_config.txt;"
  done
}

calculate_dscp() {
  local queue_num=$1
  local offset=${2:-0}
  local base_value=$((100 / queue_num))
  local remainder=$((100 % queue_num))
  local dscp_info=""

  for ((i = 0; i < queue_num; i++)); do
    local dscp_value=$((offset + i + 1))
    if [ $i -lt $remainder ]; then
      dscp_info+="dscp $dscp_value $((base_value + 1))\n"
    else
      dscp_info+="dscp $dscp_value $base_value\n"
    fi
  done

  echo -e "$dscp_info"
}

config_mq() {
  if [ $# -ne 3 ]; then
    echo "Usage: $0 queue_num_1 queue_num_2 cubic_bg"
    exit 1
  fi
  configure "$1" "$2" "$3"
}

# config 函数
config() {
  if [ $# -ne 1 ]; then
    echo "Usage: $0 queue_num"
    exit 1
  fi
  configure "$1" "$1" 0
}

config_params() {
  set-congestion-control $congestion_control
  if ((multi_queue == 1)); then
    echo "multi-queue(bg is cubic)"
    config_mq $query_queue_num $background_queue_num 1
  else
    echo "no-multiqueue"
    if ((query_queue_num != 1)); then
      echo "WARNING: they are in many queues"
    fi
    config $query_queue_num
  fi
  del_tc_config
  if ((host_scheduling == 1)); then
    echo "Host   scheduling: SP"
    sp_config
  elif ((host_scheduling == 2)); then
    echo "Host   scheduling: DRR"
    drr_config
  else
    echo "Host   scheduling: default"
  fi
  if ((switch_scheduling == 1)); then
    echo "Switch scheduling: SP"
  else
    echo "Swtich scheduling: DRR"
  fi
  arp-add
}
