#!/bin/bash
CONFIG_FILE="../general/config.conf"
if [[ -f "$CONFIG_FILE" ]]; then
  source "$CONFIG_FILE"
else
  echo "$CONFIG_FILE Not found！"
  exit 1
fi

echo "Servers IP:"
for server in ${SERVER_LIST[@]}; do
  echo " $server"
done

source ../general/tools_switch.sh
source ../general/tools_config.sh
source ../general/tools_traffic.sh
method_list=("DT" "Occamy" "ABM" "Pushout")
query_size_list=(1.5 1.6 1.7 1.8 1.9 2.0 2.1 2.2 2.3 2.4 2.5)
query_size_list=(2.2 2.3 2.4 2.5)
exp_time=120
switch_queue_num=1
pushout_seq=3
base_query_size="419430"
exp_query_load=100
exp_bg_load=11000
congestion_control=2
switch_scheduling=1
host_scheduling=1
multi_queue=1
query_queue_num=1
background_queue_num=1
exp_bak_cong="CUBIC"

exp_init_cwnd=83

exp_one_time() {
  local method=$1
  local start_time=$(date +%s)
  local query_size=$2
  local alpha=$3
  local t=$4
  local bg_flg=$5
  local str="_b"
  local s="$method$alpha"
  change-query_size $base_query_size $query_size
  set-init-cwnd $exp_init_cwnd
  while true; do
    kill_client
    sleep 2
    restart_server-single-bg $exp_bak_cong
    start_time=$(date +%s)
    start-client-one-port $t $exp_query_load $exp_bg_load $bg_flg $exp_bak_cong # >tmp.txt 2>&1
    sleep 10
    if [[ -s tmp.txt ]]; then
      echo "restart lab"
      check_switch_bug
      if [ $? -eq 1 ]; then
        echo "restart switch"
        kill_switch
        run_switch $method $alpha
        arp-add
      fi
    else
      echo "lab success"
      break
    fi
    sleep 1
  done
  sleep 10
  finish-judge-one-port
  sleep 5
  download-txt-one-port $s$str $query_size $bg_flg
  local end_time=$(date +%s)
  local total_time=$(($end_time - $start_time))
  local minutes=$(($total_time / 60))
  local seconds=$(($total_time % 60))
  echo "Time= $minutes m $seconds s"
  date "+%H:%M:%S"
}

main() {
  config_params
  echo "start"
  for method in "${method_list[@]}"; do

    # add background
    kill_switch
    run_switch $method 0
    arp-add
    for query_size in "${query_size_list[@]}"; do
      while true; do
        echo $method
        echo $query_size
        echo $exp_time
        exp_one_time $method $query_size 0 $exp_time 1
        check_switch_bug
        if [ $? -eq 0 ]; then
          break
        else
          run_switch $method 0
          arp-add
        fi
        echo "switch closed"
      done
    done
    # no background
    kill_switch
    run_switch $method 0
    arp-add
    for query_size in "${query_size_list[@]}"; do
      while true; do
        exp_one_time $method $query_size 0 $exp_time 0
        check_switch_bug
        if [ $? -eq 0 ]; then
          break
        else
          run_switch $method 0
          arp-add
        fi
        echo "switch closed"
      done
    done

  done

  echo finish
  save_params
}
main $@
