#!/bin/bash

kill_switch() {
  sudo pkill dpdk-switch
}

# run_switch [Buffer Management] [alpha]
run_switch() {
  local bm="$1"
  local arg2="$2"
  local alpha="${arg2/#f/-}" # 如果字符串以 'f' 开头，替换为 '-'

  sleep 1
  local schd="DRR"
  if ((switch_scheduling == 1)); then
    schd="PQ"
  fi
  echo "running switch BM:$bm alpha:$alpha $schd"
  nohup bash ${dpdk_switch_path}run.sh $bm $alpha $schd >switch-output.txt 2>&1 &
  sleep 15
}

#run_switch() {
#  local bm="$1"
#  local m=0
#  if [[ "$bm" == "Occamy" ]]; then
#    m=1
#  elif [[ "$bm" == "Pushout" ]]; then
#    m=3
#  elif [[ "$bm" == "ABM" ]]; then
#    m=2
#  fi
#  local arg2="$2"
#  local alpha="${arg2/#f/-}" # 如果字符串以 'f' 开头，替换为 '-'
#
#  sleep 1
#  local schd="DRR"
#  if ((switch_scheduling == 1)); then
#    schd="PQ"
#    echo "running switch BM:$bm $m alpha:$alpha $schd"
#    nohup bash ${dpdk_switch_path}run_pq.sh $m $alpha >switch-output.txt 2>&1 &
#  else
#    echo "running switch BM:$bm $m alpha:$alpha $schd"
#    nohup bash ${dpdk_switch_path}run.sh $m $alpha >switch-output.txt 2>&1 &
#  fi
#  sleep 15
#}

check_switch_bug() {
  if ps aux | grep -q '[s]witch'; then
    return 0
  else
    return 1
  fi
}
