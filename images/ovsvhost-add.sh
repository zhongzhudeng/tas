#!/usr/bin/env bash

if [ "$#" -ne 3 ]; then
    echo "Illegal number of parameters"
    echo "usage:"
    echo "[br_name vhost_name n_queues]"
    exit
fi

br_name=$1
vhost_name=$2
n_queues=$3

echo "Set bridge datapath type to netdev"
ovs-vsctl set bridge ${br_name} datapath_type=netdev

echo "Adding interface ${vhost_name} to port"
ovs-vsctl add-port ${br_name} ${vhost_name} -- set Interface ${vhost_name} options:n_rxq=$n_queues type=dpdkvhostuser