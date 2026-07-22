#!/bin/bash
set -e
TARGET=$1
if [ -z "$TARGET" ]; then
    echo "Usage: $0 /dev/sdX"
    echo "Available block devices:"
    lsblk -d -o NAME,SIZE,MODEL --noheadings 2>/dev/null
    exit 1
fi
ISO="$HOME/Vextryn_Air/vextryn-air.iso"
if [ ! -f "$ISO" ]; then echo "Error: ISO not found at $ISO"; exit 1; fi
echo "=== Vextryn Air USB Flasher ==="
echo "WARNING: ALL DATA ON $TARGET ERASED"
echo "Flashing..."
sudo dd if="$ISO" of="$TARGET" bs=4M status=progress conv=fsync
sync
sudo eject "$TARGET" 2>/dev/null || true
echo "Done! Vextryn Air flashed to $TARGET"
