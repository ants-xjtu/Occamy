#!/bin/bash
kill_client() {
  for hostip in ${SERVER_LIST[@]}; do
    ssh root@$hostip "cd  ${traffic_path}; pkill client > /dev/null 2>&1"
  done
}

start-client-control() {
  # use the default congestion control
  local t=$1
  local query_load=$2
  local bak_load=$3
  local bak_cong=$4
  for ip in ${SERVER_LIST[@]}; do
    local hid=${ip: -1}
    ssh root@$ip "ulimit -n 65535; cd ${traffic_path}; echo ''>finish_query.txt; ./bin/incast-client -b $query_load -c conf/dpdk_incast_client_config.txt -l dc$hid-flows_incast.txt -s 123 -t $t > result_incast.txt > /dev/null; echo finish > finish_query.txt" &
    ssh root@$ip "ulimit -n 65535; cd ${traffic_path}; echo ''>finish_bg.txt; ./bin/client -C $bak_cong -b $bak_load -c conf/dpdk_client_config.txt -l dc$hid-flows.txt -s 123 -t $t > result.txt > /dev/null; echo finish > finish_bg.txt" &
  done
}

start-client-one-port() {
  # query use default(DCTCP) background use CUBIC
  local t=$1
  local query_load=$2
  local bak_load=$3
  local has_bak=$4 # 0->no background traffic; 1->add background traffic
  local bak_cong=$5

  local hid=${ONE_CLIENT_IP: -1}
  ssh root@$ONE_CLIENT_IP "ulimit -n 65535; cd ${traffic_path}; echo ''>finish_query.txt; ./bin/incast-client -b $query_load -c conf/dpdk_incast_client_config.txt -l dc$hid-flows_incast.txt -s 123 -t $t > result_incast.txt > /dev/null; echo finish > finish_query.txt" &
  if (($has_bak == 1)); then
    ssh root@$ONE_CLIENT_IP "ulimit -n 65535; cd ${traffic_path}; echo ''>finish_bg.txt; ./bin/client -C $bak_cong -b $bak_load -c conf/dpdk_client_config.txt -l dc$hid-flows.txt -s 123 -t $t; echo finish > finish_bg.txt" &
  fi
}

restart_server-single-bg() {
  # one use default, the other use CUBIC
  local pathx=${traffic_path}
  local bak_cong=$1
  echo "restarting server"
  for hostip in ${SERVER_LIST[@]}; do
    ssh root@$hostip >start-server.txt 2>&1 <<eeooff
    cd $pathx
    pkill -f "bin/server"
	sleep 1
    ulimit -n 65535;./bin/server -p 5001 -d;
    ulimit -n 65535;./bin/server -p 5002 -C $bak_cong -d;
    exit
eeooff
  done
}

restart_server() {
  # use the default congestion control
  local pathx=${traffic_path}
  echo "restarting server"
  for hostip in ${SERVER_LIST[@]}; do
    ssh root@$hostip >start-server.txt 2>&1 <<eeooff
    cd $pathx
    pkill -f "bin/server"
	sleep 1
    ulimit -n 65535;./bin/server -p 5001 -d
    exit
eeooff
  done
  #echo "restart finish"
}

save_params() {
  mkdir -p result
  {
    echo "congestion_control: $congestion_control # 0:cubic 1:reno 2:dctcp"
    echo "exp_query_load: $exp_query_load # load of query traffic (e.g., 100 -> 100M, 1000 -> 1000M)"
    echo "exp_bg_load: $exp_bg_load # load of background traffic (e.g., 100 -> 100M, 1000 -> 1000M)"
    echo "multi_queue: $multi_queue # is multiqueue bit (1 -> multiqueue, 0 -> single queue)"
    echo "query_queue_num: $query_queue_num # num of query queue"
    echo "background_queue_num: $background_queue_num # num of background queue (valid only if multi_queue is 1)"
    echo "host_scheduling: $host_scheduling # scheduling algorithm of hosts (0 -> default, 1 -> sp, 2 -> drr)"
    echo "switch_scheduling: $switch_scheduling # scheduling algorithm of switch (0 -> drr, 1 -> sp)"
    echo "method_list: (${method_list[*]}) # list of buffer management methods (0 -> DT, 1 -> pBuffer, 2 -> ABM, 3 -> pushout)"
    echo "alpha_list: (${alpha_list[*]}) # list of alpha for each method (0 -> 1, 1 -> 2, 2 -> 4, 3 -> 8)"
    echo "base_query_size: $base_query_size # base size of query size"
    echo "query_size_list: (${query_size_list[*]}) # list of query size (0.5 -> 0.5 * base_query_size)"
    echo "exp_time: $exp_time # time of each exp (e.g., 180 -> 180s)"
    echo "exp_init_cwnd: $exp_init_cwnd"
  } >result/params.txt
}

check_file() {
  ssh "$1" "grep -q 'finish' \"$2\""
  return $?
}

finish-judge-one-port() {
  local file_path="${traffic_path}finish_query.txt"
  local max_wait_time=$((2 * exp_time))
  local elapsed_time=0

  # local ONE_CLIENT_IP="192.168.1.8"

  while true; do
    local all_finished=true

    if ! check_file "root@$ONE_CLIENT_IP" "$file_path"; then
      all_finished=false
    fi

    if $all_finished; then
      echo "finish"
      break
    fi

    sleep 5
    elapsed_time=$((elapsed_time + 5))

    if [ $elapsed_time -ge $max_wait_time ]; then
      echo "timeout"
      return 1
    fi
  done
  ssh root@$ONE_CLIENT_IP "echo "" > $file_path"
}

finish-judge() {
  local file_path="${traffic_path}finish_bg.txt"
  local file_path2="${traffic_path}finish_query.txt"
  local max_wait_time=$((2 * exp_time)) # t 是传递给该函数的参数

  local elapsed_time=0 # 初始化已经过去的时间

  while true; do
    local all_finished=true

    for hostip in "${SERVER_LIST[@]}"; do
      if ! check_file "root@$hostip" "$file_path"; then
        all_finished=false
        break
      fi
      if ! check_file "root@$hostip" "$file_path2"; then
        all_finished=false
        break
      fi
    done

    if $all_finished; then
      echo "finish"
      break
    fi

    sleep 5
    elapsed_time=$((elapsed_time + 5))

    if [ $elapsed_time -ge $max_wait_time ]; then
      echo "timeout"
      return 1
    fi
  done

  for hostip in "${SERVER_LIST[@]}"; do
    ssh root@$hostip "echo "" > $file_path"
    ssh root@$hostip "echo "" > $file_path2"
  done
}

download-txt-one-port() {
  local filename="$1-$2"
  local has_bak=$3
  if (($has_bak == 1)); then
    filename="${1}_b-$2"
  fi
  echo "the file name is $filename"
  mkdir -p result/$filename
  local path=$traffic_path
  #local hip="192.168.1.8"
  local hip=$ONE_CLIENT_IP
  local hid=${hip: -1}
  scp root@$hip:${path}dc${hid}-flows.txt ./result/$filename >/dev/null
  scp root@$hip:${path}dc${hid}-flows_incast.txt_flows.txt ./result/$filename >/dev/null
  scp root@$hip:${path}dc${hid}-flows_incast.txt_reqs.txt ./result/$filename
}

download-txt() {
  local filename="$1$3-$2"
  echo "the file name is $filename"
  mkdir -p result/$filename
  local path=$traffic_path

  for hip in ${SERVER_LIST[@]}; do
    local hid=${hip: -1}
    scp root@$hip:${path}dc$hid-flows.txt ./result/$filename >/dev/null
    scp root@$hip:${path}dc$hid-flows_incast.txt_flows.txt ./result/$filename >/dev/null
    if ((hid == ${SERVER_LIST[0]: -1})); then
      scp root@$hip:${path}dc$hid-flows_incast.txt_reqs.txt ./result/$filename
    else
      scp root@$hip:${path}dc$hid-flows_incast.txt_reqs.txt ./result/$filename >/dev/null
    fi
  done
}
