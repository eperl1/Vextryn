#!/bin/bash
while [ ! -f /tmp/vxair_agent4_done ]; do sleep 2; done
cd ~/Vextryn_Air
source .env
echo "[Agent 5] Booting QEMU headless..."
timeout 20 qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -smp 2 -serial file:/tmp/vxair_serial.log -display none -no-reboot 2>/dev/null &
QEMU_PID=$!
sleep 10
kill $QEMU_PID 2>/dev/null
touch /tmp/vxair_agent5_done
