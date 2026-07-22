#!/bin/bash
if command -v apt &>/dev/null; then 
    sudo apt update -y 
    sudo DEBIAN_FRONTEND=noninteractive apt install -y build-essential cmake ninja-build python3 python3-pip nasm gcc g++ binutils xorriso mtools ovmf qemu-system-x86 qemu-utils git curl wget gdb libgmp-dev libmpfr-dev libmpc-dev flex bison texinfo grub-pc-bin grub-efi-amd64-bin
fi
OVMF_PATHS=("/usr/share/ovmf/OVMF.fd" "/usr/share/OVMF/OVMF.fd" "/usr/share/edk2/ovmf/OVMF_CODE.fd" "/usr/share/qemu/OVMF.fd")
for path in "${OVMF_PATHS[@]}"; do if [ -f "$path" ]; then OVMF_FD="$path"; break; fi; done
if [ -z "$OVMF_FD" ]; then sudo apt install -y ovmf && OVMF_FD="/usr/share/ovmf/OVMF.fd"; fi
echo "OVMF_FD=$OVMF_FD" > ~/Vextryn_Air/.env
touch /tmp/vxair_agent1_done
echo "[Agent 1] Dependencies installed."
