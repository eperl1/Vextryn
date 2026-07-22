#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_out"
ISO_DIR="${BUILD_DIR}/iso_root"
ISO_FILE="${BUILD_DIR}/vextryn_air.iso"
KERNEL_BIN="${BUILD_DIR}/bin/vextryn_air.elf"

echo "Creating ISO directory structure..."
mkdir -p "${ISO_DIR}/boot/grub"

if [ -f "${KERNEL_BIN}" ]; then
    cp "${KERNEL_BIN}" "${ISO_DIR}/boot/vextryn_air.elf"
else
    echo "Warning: Kernel binary not found at ${KERNEL_BIN}."
    echo "Creating a dummy kernel file for ISO structure..."
    touch "${ISO_DIR}/boot/vextryn_air.elf"
fi

echo "Creating GRUB config..."
cat > "${ISO_DIR}/boot/grub/grub.cfg" << EOF
menuentry "Vextryn Air OS" {
    multiboot /boot/vextryn_air.elf
    boot
}
EOF

echo "Generating ISO..."
# Requires mtools and xorriso to be installed
# Ensure grub-mkrescue dependencies are met in environment
if command -v grub-mkrescue &> /dev/null; then
    grub-mkrescue -o "${ISO_FILE}" "${ISO_DIR}"
    echo "ISO created at ${ISO_FILE}"
else
    echo "Error: grub-mkrescue not found. Please install grub-common / grub-pc-bin / xorriso."
    exit 1
fi
