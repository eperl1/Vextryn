#!/bin/bash
# Vextryn Air OS - Local QEMU Run Script
cd "$(dirname "$0")/.."
qemu-system-x86_64 \
    -cdrom build_out/vextryn_air.iso \
    -m 5120M -smp 4 \
    -machine q35 -enable-kvm -cpu host \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -device qemu-xhci,id=xhci -device usb-host,vendorid=0x2001,productid=0x332c,bus=xhci.0 \
    -serial file:/tmp/vxair_final.log \
    -vga std -no-reboot
