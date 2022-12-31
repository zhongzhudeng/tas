#!/usr/bin/env bash

set -ex

machineName=$1
option=$2
num=$3
cnt=0

STTY_SETTINGS="$( stty -g )"

stty intr ^]
stty susp ^]

cnt=$(($cnt+1))
# Parameters.
id=ubuntu-18.04.6-desktop-amd64
disk_img="base.img"
seed_img="seed.img"
disk_img_snapshot="${machineName}.snapshot.qcow2"

echo "NUM = ${num}"

if [[ $option == proxy ]]; then
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp 16 \
    -m 12G \
    -snapshot \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::222${num}-:22 \
    -drive if=virtio,format=qcow2,file="base.snapshot.qcow2" \
    -drive if=virtio,format=raw,file="seed.img" \
    -chardev socket,path="/run/tasproxy",id="tas" \
    -device ivshmem-doorbell,vectors=1,chardev="tas" \
  ;
elif [[ $option == tap ]]; then
  sudo qemu-system-x86_64 \
    -nographic -monitor none -serial stdio \
    -machine accel=kvm,type=q35 \
    -cpu host \
    -smp 16 \
    -m 12G \
    -snapshot \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::222${num}-:22 \
    -netdev tap,ifname=tap0,script=no,downscript=no,vhost=on,id=nInt\
    -device virtio-net-pci,mac=52:54:00:12:34:56,netdev=nInt \
    -drive if=virtio,format=qcow2,file="base.snapshot.qcow2" \
    -drive if=virtio,format=raw,file="seed.img" \
  ;
fi

stty "$STTY_SETTINGS"
