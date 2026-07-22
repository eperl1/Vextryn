#!/bin/bash
# Simplest possible QEMU boot for debugging
qemu-system-x86_64 \
  -cdrom ~/Vextryn_Air/vextryn-air.iso \
  -m 512M \
  -vga std \
  -display sdl \
  -serial stdio \
  -no-reboot
