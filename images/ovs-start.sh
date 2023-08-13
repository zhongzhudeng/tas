#!/usr/bin/env bash

/usr/local/share/openvswitch/scripts/ovs-ctl start
echo "Setting cpu mask"
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=0x15