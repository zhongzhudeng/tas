#!/usr/bin/env bash
pmd_mask=$1

export PATH=$PATH:/usr/local/share/openvswitch/scripts
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock

echo "Starting OvS DB"
ovs-ctl --no-ovs-vswitchd start

echo "Setting dpdk-init to true"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
echo "Setting hugepages dir"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-hugepages-dir=/dev/hugepages
echo "Setting cpu mask"
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=$pmd_mask

echo "Starting ovs switch"
ovs-ctl --no-ovsdb-server --db-sock="$DB_SOCK" start