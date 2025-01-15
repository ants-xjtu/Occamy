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

method_list=("DT")
alpha_list=(0 1 2 3 f1)
query_size_list=(1.1 1.2 1.3 1.4 1.5 1.6)
exp_time=120
switch_queue_num=1
pushout_seq=3
base_query_size="419430"
exp_query_load=100
exp_bg_load=4000
congestion_control=2
switch_scheduling=0
host_scheduling=0
query_queue_num=1
multi_queue=1
background_queue_num=1
exp_bak_cong="DEFAULT"

exp_init_cwnd=83

exp_one_time() {
  local method=$1
  local start_time=$(date +%s)
  local query_size=$2
  local alpha=$3
  local t=$4

  change-query_size $base_query_size $query_size
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
  download-txt $method $query_size $alpha
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
    for alpha in "${alpha_list[@]}"; do
      if [ "$method" = "$pushout_seq" ] && [ "$alpha" -ne "${alpha_list[0]}" ]; then
        continue
      fi
      kill_switch
      run_switch $method $alpha
      arp-add
      for query_size in "${query_size_list[@]}"; do
        while true; do
          check_switch_bug
          exp_one_time $method $query_size $alpha $exp_time
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
  done

  echo finish
  save_params
}
main $@
