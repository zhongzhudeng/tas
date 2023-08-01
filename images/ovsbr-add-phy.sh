#!/usr/bin/env bash

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    echo "usage:"
    echo "[br_name mac_address]"
    exit
fi

br_name=$1

# Mac address of physical interface (e.g. the Mellanox NIC)
mac=$2

ovs-vsctl --may-exist add-br $br_name \
    -- set Bridge $br_name datapath_type=netdev \
    -- br-set-external-id $br_name bridge-id $br_name \
    -- set bridge $br_name fail-mode=standalone \
         other_config:hwaddr=$mac