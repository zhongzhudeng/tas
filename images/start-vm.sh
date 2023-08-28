#!/usr/bin/env bash

set -ex

stack=$1
vm_id=$2
interface=$3
n_cores=$4
memory=$5 # In Gigabytes
cores_tag=$6
n_queues=$7

stty intr ^]
stty susp ^]

STTY_SETTINGS="$( stty -g )"

# Parameters.
disk_img="base.img"
seed_img="seed.img"
disk_img_snapshot="base.snapshot.qcow2"
tap=tap$vm_id
tastap=tastap$vm_id
ovstap=ovstap$vm_id
vhost=vhost$vm_id



socket0_cores=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42
socket0_cores_1sthalf=0,2,4,6,8,10,12,14,16,18,20
socket0_cores_2ndhalf=22,24,26,28,30,32,34,36,38,40,42

socket1_cores=1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43
socket1_cores_1sthalf=1,3,5,7,9,11,13,15,17,19,21
socket1_cores_2ndhalf=23,25,27,29,31,33,35,37,39,41,43

if [[ "$cores_tag" == 'socket0_cores' ]]; then
  core_mask=$socket0_cores
elif [[ "$cores_tag" == 'socket0_cores_1sthalf' ]]; then
  core_mask=$socket0_cores_1sthalf
elif [[ "$cores_tag" == 'socket0_cores_2ndhalf' ]]; then
  core_mask=$socket0_cores_2ndhalf
elif [[ "$cores_tag" == 'socket1_cores' ]]; then
  core_mask=$socket1_cores
elif [[ "$cores_tag" == 'socket1_cores_1sthalf' ]]; then
  core_mask=$socket1_cores_1sthalf
elif [[ "$cores_tag" == 'socket0_cores_2ndhalf' ]]; then
  core_mask=$socket1_cores_2ndhalf
else
  echo "NO MATCH FOR CORES_TAG!"
fi

if [ -n "$n_queues" ]; then
  vectors=$(( n_queues*2 + 2 ))
fi

printf -v mac '02:00:00:%02X:%02X:%02X' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256))
printf -v alt_mac '02:00:00:%02X:%02X:%02X' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256))

echo $mac
echo $alt_mac

# Note: vectors=<2 + 2 * queues_nr>

if [[ "$stack" == 'virt-tas' ]]; then
  taskset -c $core_mask \
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp $n_cores \
    -m ${memory}G \
    -snapshot \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::222${vm_id}-:22 \
    -chardev socket,path="/run/tasproxy",id="tas" \
    -device ivshmem-doorbell,vectors=1,chardev="tas" \
    -drive if=virtio,format=raw,file="base.img" \
    -drive if=virtio,format=raw,file="seed.img" \
    ;
elif [[ "$stack" == 'virt-linux' ]]; then
  taskset -c $core_mask \
  sudo qemu-system-x86_64 \
      -nographic -monitor none -serial stdio \
      -machine accel=kvm,type=q35 \
      -cpu host \
      -smp $n_cores \
      -m ${memory}G \
      -snapshot \
      -netdev user,id=net0 \
      -device virtio-net-pci,netdev=net0 \
      -netdev tap,ifname=$tap,script=no,downscript=no,vhost=on,id=net1 \
      -device virtio-net-pci,mac=$mac,netdev=net1 \
      -drive if=virtio,format=raw,file="base.img" \
      -drive if=virtio,format=raw,file="seed.img" \
      ;
elif [[ "$stack" == 'ovs-linux' ]]; then
  taskset -c $core_mask \
    sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp $n_cores \
    -m ${memory}G \
    -snapshot \
    -netdev user,id=net0,hostfwd=tcp::222${vm_id}-:22 \
    -device virtio-net-pci,netdev=net0 \
    -chardev socket,id=char0,path=/usr/local/var/run/openvswitch/$vhost \
    -netdev type=vhost-user,chardev=char0,vhostforce=on,queues=$n_queues,id=net1 \
    -device virtio-net-pci,netdev=net1,mac=$alt_mac,mq=on,vectors=$vectors \
    -object memory-backend-file,id=mem,size=10G,mem-path=/dev/hugepages,share=on \
    -numa node,memdev=mem -mem-prealloc \
    -drive if=virtio,format=raw,file="base.img" \
    -drive if=virtio,format=raw,file="seed.img" \
    ;
elif [[ "$stack" == 'ovs-tas' ]]; then
  taskset -c $core_mask \
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp $n_cores \
    -m ${memory}G \
    -snapshot \
    -netdev user,id=net0,hostfwd=tcp::222${vm_id}-:22 \
    -device virtio-net-pci,netdev=net0 \
    -chardev socket,id=char0,path=/usr/local/var/run/openvswitch/$vhost \
    -netdev type=vhost-user,chardev=char0,vhostforce=on,queues=$n_queues,id=net1 \
    -device virtio-net-pci,netdev=net1,mac=$alt_mac,mq=on,vectors=$vectors,rss=on,hash=on \
    -object memory-backend-file,id=mem,size=10G,mem-path=/dev/hugepages,share=on \
    -numa node,memdev=mem -mem-prealloc \
    -drive if=virtio,format=raw,file="base.img" \
    -drive if=virtio,format=raw,file="seed.img" \
    ;
elif [[ "$stack" == 'tap-tas' ]]; then
  taskset -c $core_mask \
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp $n_cores \
    -m ${memory}G \
    -snapshot \
    -netdev user,id=net0 \
    -device virtio-net-pci,netdev=net0 \
    -netdev tap,ifname=$tap,script=no,downscript=no,vhost=on,id=net1 \
    -device virtio-net-pci,mac=$mac,netdev=net1 \
    -netdev tap,ifname=$tastap,script=no,downscript=no,vhost=on,queues=$n_queues,id=net2 \
    -device virtio-net-pci,mac=$alt_mac,vectors=$vectors,mq=on,netdev=net2 \
    -drive if=virtio,format=raw,file="base.img" \
    -drive if=virtio,format=raw,file="seed.img" \
    ;
elif [[ "$stack" == 'gre' ]]; then
  taskset -c $core_mask \
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp $n_cores \
    -m ${memory}G \
    -snapshot \
    -netdev user,id=net0 \
    -device virtio-net-pci,netdev=net0 \
    -netdev bridge,br=br2,id=net1 \
    -device virtio-net-pci,id=nic1,netdev=net1 \
    -drive if=virtio,format=raw,file="base.img" \
    -drive if=virtio,format=raw,file="seed.img" \
    ;
fi

stty "$STTY_SETTINGS"
