#!/bin/bash
# Vextryn Air QEMU GUI Launch
MEM=${1:-512}

# Find OVMF
OVMF=""
for p in /usr/share/ovmf/OVMF.fd \
          /usr/share/OVMF/OVMF.fd \
          /usr/share/edk2-ovmf/x64/OVMF.fd \
          /usr/share/qemu/OVMF.fd; do
  if [ -f "$p" ]; then
    OVMF="-bios $p"
    echo "Using OVMF: $p"
    break
  fi
done

echo "Starting Vextryn Air..."
echo "Serial output will appear here"
echo "================================"

qemu-system-x86_64 \
  $OVMF \
  -cdrom ~/Vextryn_Air/vextryn-air.iso \
  -m ${MEM}M \
  -smp 2 \
  -machine q35 \
  -cpu qemu64 \
  -device virtio-net-pci,netdev=net0 \
  -netdev user,id=net0 \
  -vga std \
  -display sdl \
  -serial stdio \
  -drive file=/home/ethan/Vextryn_Air/vextryn-data.img,format=raw,index=1,media=disk \
  -no-reboot \
  "$@"
