#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISO_FILE="${PROJECT_ROOT}/build_out/vextryn_air.iso"
VM_NAME="VextrynAirOS"

if [ ! -f "${ISO_FILE}" ]; then
    echo "Error: ISO not found at ${ISO_FILE}."
    exit 1
fi

echo "Setting up VirtualBox VM ${VM_NAME}..."

# Check if VM already exists, if so, power it off if running
if VBoxManage showvminfo "${VM_NAME}" &> /dev/null; then
    echo "VM already exists."
    # Power off if running
    if VBoxManage showvminfo "${VM_NAME}" --machinereadable | grep -q 'VMState="running"'; then
        VBoxManage controlvm "${VM_NAME}" poweroff
        sleep 2
    fi
else
    echo "Creating new VM..."
    VBoxManage createvm --name "${VM_NAME}" --ostype "Other_64" --register
    VBoxManage modifyvm "${VM_NAME}" --memory 512 --vram 16
    VBoxManage storagectl "${VM_NAME}" --name "IDE Controller" --add ide
fi

echo "Attaching ISO and starting VM..."
VBoxManage storageattach "${VM_NAME}" --storagectl "IDE Controller" --port 0 --device 0 --type dvddrive --medium "${ISO_FILE}"
VBoxManage startvm "${VM_NAME}" --type gui
