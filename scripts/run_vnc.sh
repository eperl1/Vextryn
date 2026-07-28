#!/bin/bash
echo "Starting Vextryn Air with VNC..."
echo "Connect with: vncviewer localhost:5900"
qemu-system-x86_64 \
  -cdrom ~/Vextryn_Air/vextryn-air.iso \
  -m 5120M \
  -vga std \
  -display vnc=:0 \
  -serial stdio \
  -no-reboot
