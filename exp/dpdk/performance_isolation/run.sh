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
alpha_list=(0 3 1 0)
bg_load_list=(1000 2000 3000 4000 5000 6000)
exp_time=180
switch_queue_num=1
pushout_seq=3
base_query_size="419430"
exp_query_load=100
exp_bg_load=6000
congestion_control=2
switch_scheduling=0
host_scheduling=0
multi_queue=1
query_queue_num=1
background_queue_num=1
exp_query_size=1.5

exp_bak_cong="CUBIC"

exp_init_cwnd=83

exp_one_time() {
  local method=$1
  local start_time=$(date +%s)
  #local query_size=$2
  local exp_bg_load=$2
  local alpha=$3
  local t=$4
  local s="$method$alpha"

  #change-query_size $base_query_size $query_size
  set-init-cwnd $exp_init_cwnd
  while true; do
    kill_client
    sleep 2
    restart_server-single-bg $exp_bak_cong
    start_time=$(date +%s)

    start-client-control $t $exp_query_load $exp_bg_load $exp_bak_cong >tmp.txt 2>&1
    sleep 10
    if [[ -s tmp.txt ]]; then
      echo "restart lab"
      check_switch_bug
      arp-add
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
  sleep $t
  finish-judge
  sleep 5
  download-txt $method $exp_bg_load $alpha
  local end_time=$(date +%s)
  local total_time=$(($end_time - $start_time))
  local minutes=$(($total_time / 60))
  local seconds=$(($total_time % 60))
  echo "Time= $minutes m $seconds s"
  date "+%H:%M:%S"
}

main() {
  config_params
  change-query_size $base_query_size $exp_query_size
  echo "start"
  for methodi in "${!method_list[@]}"; do
    #for method in "${method_list[@]}"; do
    local method=${method_list[$methodi]}
    local alpha=${alpha_list[$methodi]}

    kill_switch
    run_switch $method $alpha
    arp-add
    for bg_load in "${bg_load_list[@]}"; do
      while true; do
        check_switch_bug
        exp_one_time $method $bg_load $alpha $exp_time
        if [ $? -eq 0 ]; then
          break
        else
          kill_switch
          run_switch $method $alpha
          arp-add
        fi
        echo "ERR:  #####   switch closed    #####"
      done
    done
  done

  echo finish
  save_params
}
main $@
