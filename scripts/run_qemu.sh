#!/bin/bash
# Vextryn Air OS - Local QEMU Run Script
cd "$(dirname "$0")/.."
qemu-system-x86_64 \
    -cdrom build_out/vextryn_air.iso \
    -m 5120M -smp 4 \
    -machine q35 -enable-kvm -cpu host \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -serial file:/tmp/vxair_final.log \
    -vga std -no-reboot
