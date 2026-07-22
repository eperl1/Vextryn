#!/bin/bash
cd ~/Vextryn_Air/build-out
cmake --build . --parallel $(nproc)
if [ $? -ne 0 ]; then
    echo "BUILD FAILED"
    exit 1
fi

cd ~/Vextryn_Air
rm -rf iso_root && mkdir -p iso_root/EFI/BOOT iso_root/vextryn iso_root/boot/grub
cp build-out/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
cat > iso_root/boot/grub/grub.cfg << 'GRUB_EOF'
set timeout=0
set default=0
menuentry "Vextryn Air" { multiboot2 /vextryn/kernel.elf; boot }
GRUB_EOF
grub-mkrescue -o vextryn-air.iso iso_root/ 2>/dev/null

rm -f /tmp/vxair_serial.log /tmp/qemu_int.log
source .env
timeout 3 qemu-system-x86_64 \
    -cdrom vextryn-air.iso \
    -m 512M \
    -serial file:/tmp/vxair_serial.log \
    -display none \
    -no-reboot \
    -d int,cpu_reset \
    -D /tmp/qemu_int.log &
QEMU_PID=$!
sleep 2
kill $QEMU_PID 2>/dev/null

echo "===== SERIAL LOG ====="
cat /tmp/vxair_serial.log
echo "======================"

if grep -q "vxsh$" /tmp/vxair_serial.log; then
    echo "🎉 SHELL REACHED — MISSION COMPLETE"
else
    echo "Last line: $(tail -1 /tmp/vxair_serial.log)"
    echo "Continuing fixes..."
fi
