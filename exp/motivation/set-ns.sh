#!/bin/bash

main() {

  source general/config.conf
  echo $SERVER_LIST
  for hip in ${SERVER_LIST[@]}; do
    echo $hip
    ssh root@$hip "ip netns delete ns1>/dev/null; ip netns delete ns2>/dev/null;" >/dev/null
    sleep 1
    ssh root@$hip "ip netns add ns1; ip netns add ns2;"

    local hid="${hip: -1}"
    local nic0="enp7s0f0"
    local nic1="enp7s0f1"
    if [ "$hid" = "2" ]; then
      nic0="enp3s0f0np0"
      nic1="enp3s0f1np1"
    fi
    ssh root@$hip "ip link set $nic0 netns ns1"
    ssh root@$hip "ip link set $nic1 netns ns2"

    ssh root@$hip "ip netns exec ns1 ip link set $nic0 up"
    ssh root@$hip "ip netns exec ns2 ip link set $nic1 up"
    ssh root@$hip "ip netns exec ns1 ip link set lo up"
    ssh root@$hip "ip netns exec ns2 ip link set lo up"

    ssh root@$hip "ip netns exec ns1 ip addr add 192.168.40.${hid}0/24 dev $nic0"
    ssh root@$hip "ip netns exec ns2 ip addr add 192.168.40.${hid}1/24 dev $nic1"
  done

}

main $@
