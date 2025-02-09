#!/bin/bash
set -x
source ../general/config.conf

query_sizes=(1 2 3 4 5 6 7 8 9 10 11 12 13 14)
base_query_size="1M"
host_scheduling=1

set-congestion-control() {
  # set to DCTCP
  for hip in ${SERVER_LIST[@]}; do
    ssh root@$hip "sudo ip netns exec ns1 sysctl -w net.ipv4.tcp_congestion_control=dctcp; sudo ip netns exec ns1 sysctl -w net.ipv4.tcp_ecn=1;"
    ssh root@$hip "sudo ip netns exec ns2 sysctl -w net.ipv4.tcp_congestion_control=dctcp; sudo ip netns exec ns2 sysctl -w net.ipv4.tcp_ecn=1;"
  done
}

run-background() {
  # set iperf
  pkill iperf
  sleep 2

  ssh root@192.168.1.3 "pkill iperf"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6666 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6667 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6668 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6669 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6610 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6611 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6612 -D"

  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6613 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6614 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6615 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6616 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6617 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6618 -D"
  ssh root@192.168.1.3 "ip netns exec ns1 iperf3 -s -p 6619 -D"

  ssh root@${ONE_CLIENT_IP} "pkill iperf"
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6666 -t 0 -S 32 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6667 -t 0 -S 64 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6668 -t 0 -S 96 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6669 -t 0 -S 128 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6610 -t 0 -S 160 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6611 -t 0 -S 192 &" >/dev/null &
  nohup ssh root@${ONE_CLIENT_IP} "nohup ip netns exec ns2 iperf3 -C dctcp -c 192.168.40.30 -p 6612 -t 0 -S 224 &" >/dev/null &

  ssh root@192.168.1.4 "pkill iperf"
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6613 -t 0 -S 32 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6614 -t 0 -S 64 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6615 -t 0 -S 96 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6616 -t 0 -S 128 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6617 -t 0 -S 160 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6618 -t 0 -S 192 &" >/dev/null &
  nohup ssh root@192.168.1.4 "nohup ip netns exec ns1 iperf3 -C dctcp -c 192.168.40.30 -p 6619 -t 0 -S 224 &" >/dev/null &
}

finish() {
  ssh root@${ONE_CLIENT_IP} "pkill iperf"
  ssh root@192.168.1.3 "pkill iperf"
  ssh root@192.168.1.4 "pkill iperf"
  ssh root@192.168.1.5 "pkill iperf"

  ssh root@${ONE_CLIENT_IP} "pkill 'server'"
  ssh root@192.168.1.3 "pkill 'server'"
  ssh root@192.168.1.4 "pkill 'server'"
  ssh root@192.168.1.5 "pkill 'server'"
}

set-query-size() {
  local query_size=$1
  ./set-query-size.sh $query_size
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
  ssh root@${ONE_CLIENT_IP} "echo -e '$line1\n$line2' > ${traffic_path}conf/INCAST_CDF.txt"
}

run-incast-server() {
  ssh root@192.168.1.5 "pkill 'server'"
  ssh root@192.168.1.3 "pkill 'server'"
  ssh root@192.168.1.4 "pkill 'server'"
  ssh root@${ONE_CLIENT_IP} "pkill 'server'"

  sleep 5
  ssh root@192.168.1.3 "ip netns exec ns2 ${traffic_path}bin/server -C DCTCP -p 6001 -d"
  ssh root@192.168.1.3 "ip netns exec ns1 ${traffic_path}bin/server -C DCTCP -p 6001 -d"
  ssh root@192.168.1.4 "ip netns exec ns2 ${traffic_path}bin/server -C DCTCP -p 6001 -d"
  ssh root@192.168.1.5 "ip netns exec ns1 ${traffic_path}bin/server -C DCTCP -p 6001 -d"
  ssh root@192.168.1.5 "ip netns exec ns2 ${traffic_path}bin/server -C DCTCP -p 6001 -d"

}

set-init-cwnd() {
  echo "Set init-cwnd = 699"
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}
    local hid=${hostip: -1}
    local nic0="enp7s0f0"
    local nic1="enp7s0f0"
    if [ "$hid" = "2" ]; then
      nic0="${ONE_CLIENT_NIC0}"
      nic1="${ONE_CLIENT_NIC1}"
    fi

    ssh root@$hostip "ip netns exec ns1 ip route change 192.168.40.0/24 dev $nic0 proto kernel scope link src 192.168.40.${hid}0 initcwnd 699"
    ssh root@$hostip "ip netns exec ns2 ip route change 192.168.40.0/24 dev $nic1 proto kernel scope link src 192.168.40.${hid}1 initcwnd 699"
  done
}

run-incast() {
  mkdir -p result
  ssh root@${ONE_CLIENT_IP} "ip netns exec ns1 ${traffic_path}bin/incast-client -b 1000 -C DCTCP -c ${traffic_path}conf/incast_client_config.txt -l ${traffic_path}result/query-result-$query_size.txt -s 123 -t 60"
}

download-result() {
  local method=$1
  local query_size=$2
  mkdir -p result/${method}-$query_size
  scp -r root@${ONE_CLIENT_IP}:${traffic_path}result/query-result-${query_size}* result/${method}-${query_size}/.
}

del_tc_config() {
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}

    local nic0="enp7s0f0"
    local nic1="enp7s0f0"
    if [ "$hid" = "2" ]; then
      nic0="${ONE_CLIENT_NIC0}"
      nic1="${ONE_CLIENT_NIC1}"
    fi

    ssh root@$hostip "ip netns exec ns1 tc qdisc del dev $nic0 root;" >/dev/null 2>&1
    ssh root@$hostip "ip netns exec ns2 tc qdisc del dev $nic1 root;" >/dev/null 2>&1
  done
}

configure() {
  local path="${traffic_path}conf/"
  local fanout=40

  local query_info=""

  query_info+="server 192.168.40.31 6001 1\n"
  query_info+="server 192.168.40.30 6001 1\n"
  query_info+="server 192.168.40.41 6001 1\n"
  query_info+="server 192.168.40.50 6001 1\n"
  query_info+="server 192.168.40.51 6001 1\n"
  query_info+="req_size_dist ${traffic_path}conf/INCAST_CDF.txt\n"

  query_info+="$(calculate_dscp)\n"

  query_info+="rate 0Mbps 100\nfanout $fanout 100"

  ssh root@${ONE_CLIENT_IP} "echo -e '$query_info' > ${path}incast_client_config.txt;"
}

calculate_dscp() {
  echo -e "dscp 2 100"
}

sp_config() {
  for ((i = 0; i < ${#SERVER_LIST[@]}; i++)); do
    local hostip=${SERVER_LIST[$i]}
    local hid=${hostip: -1}
    local nic0="enp7s0f0"
    local nic1="enp7s0f0"
    if [ "$hid" = "2" ]; then
      nic0="${ONE_CLIENT_NIC0}"
      nic1="${ONE_CLIENT_NIC1}"
    fi
    ssh root@hostip "tc qdisc del dev $nic0 root; tc qdisc add dev $nic0 root handle 1: prio" >/dev/null 2>&1
    ssh root@hostip "tc qdisc del dev $nic1 root; tc qdisc add dev $nic1 root handle 1: prio" >/dev/null 2>&1
  done
}

config_params() {
  # set-congestion-control
  set-init-cwnd
  configure
  del_tc_config
  # if ((host_scheduling == 1)); then
  #   echo "Host   scheduling: SP"
  sp_config
  # elif ((host_scheduling == 2)); then
  #   echo "Host   scheduling: DRR"
  #   drr_config
  # else
  #   echo "Host   scheduling: default"
  # fi
}

main() {
  config_params

  for query_size in ${query_sizes[@]}; do
    echo $query_size
    change-query_size $base_query_size $query_size
    run-incast-server
    sleep 5
    run-incast $query_size
    sleep 5
    download-result DT $query_size
  done

  finish
}

main $@
