#!/bin/bash
for i in {1..5}; do
  killall qemu-system-x86_64 2>/dev/null
  make run > /tmp/vxair_final.log 2>&1 &
  sleep 15
  if grep -q "RTL8852" /tmp/vxair_final.log; then
    echo "Found RTL8852 in log!"
    exit 0
  fi
  echo "Attempt $i failed, retrying..."
done
