#!/bin/bash
# Vextryn Air OS - Final QEMU Run Script (Headless CI)
cd "$(dirname "$0")/.."
timeout 120 qemu-system-x86_64 \
    -cdrom vextryn-air.iso \
    -m 512M -smp 4 \
    -machine q35 -cpu qemu64 \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -serial file:/tmp/vxair_final.log \
    -display none -no-reboot
