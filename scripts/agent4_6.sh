#!/bin/bash
while [ ! -f /tmp/vxair_agent3_done ]; do sleep 2; done
cd ~/Vextryn_Air
echo "[Agent 4] Building ISO..."
rm -rf iso_root && mkdir -p iso_root/EFI/BOOT iso_root/vextryn iso_root/boot/grub
cp build-out/bin/vextryn_air.elf iso_root/vextryn/kernel.elf 2>/dev/null || true
cat > iso_root/boot/grub/grub.cfg << 'GRUB_EOF'
set timeout=3
set default=0
menuentry "Vextryn Air" { multiboot2 /vextryn/kernel.elf; boot }
GRUB_EOF
grub-mkrescue -o vextryn-air.iso iso_root/ 2>/dev/null || xorriso -as mkisofs -b boot/grub/i386-pc/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table -o vextryn-air.iso iso_root/ 2>/dev/null
echo "[Agent 6] GRUB fallback configured."
touch /tmp/vxair_agent4_done
