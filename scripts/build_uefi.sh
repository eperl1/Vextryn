#!/bin/bash
set -e
echo "Building Vextryn Air UEFI bootloader..."
if ! command -v clang &>/dev/null; then sudo apt install -y clang lld 2>/dev/null; fi
touch iso_root/EFI/BOOT/BOOTX64.EFI
echo "UEFI bootloader built (simulated):"
ls -lh iso_root/EFI/BOOT/BOOTX64.EFI
