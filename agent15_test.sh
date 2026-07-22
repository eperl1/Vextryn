#!/bin/bash
cd ~/Vextryn_Air
echo "Building..."
cd build-out
cmake --build . --parallel $(nproc) 2>&1 | tee /tmp/p5_build.log
cd ..
cp build-out/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/ 2>/dev/null
echo "ISO: $(du -h vextryn-air.iso | cut -f1)"

echo "Booting..."
rm -f /tmp/vxair_p5.log
# Run QEMU in background and kill after 5 seconds to get log output properly flushed
timeout 5 qemu-system-x86_64 \
    -cdrom vextryn-air.iso \
    -m 512M -smp 4 \
    -machine q35 -cpu qemu64 \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -serial file:/tmp/vxair_p5.log \
    -display none -no-reboot -d cpu_reset 2>/dev/null || true
# give it a moment to flush
sleep 1

PASS=0
FAIL=0
for M in \
    "Welcome to Vextryn Air" \
    "PMM" "VMM" "KHeap" \
    "IDT" "APIC" "Syscall" \
    "Scheduler" "IPC" \
    "Kernel Core initialized" \
    "VFS: root mounted" \
    "INITRD: loaded" \
    "eth0" \
    "compositor" \
    "vxsh"; do
    if grep -q "$M" /tmp/vxair_p5.log 2>/dev/null; then
        echo "✅ $M"
        PASS=$((PASS+1))
    else
        echo "❌ MISSING: $M"
        FAIL=$((FAIL+1))
    fi
done
echo "Score: $PASS/15"
echo "=== SERIAL LOG ==="
cat /tmp/vxair_p5.log 2>/dev/null || echo "(empty)"
echo "=================="

if [ $PASS -eq 15 ]; then
    echo "ALL MILESTONES PASSED"
    
    FILE_COUNT=$(find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.asm" | grep -v build-out | wc -l)
    LOC=$(find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.asm" | grep -v build-out | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
    TODO=$(grep -r "TODO\|FIXME\|STUB" . --include="*.c" --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
    ISO=$(du -h vextryn-air.iso 2>/dev/null | cut -f1 || echo "NOT FOUND")

    cat > docs/phase5_report.md << REPORT
# Vextryn Air v0.1.0 — Phase 5 Report

## Stats
- Source files: $FILE_COUNT
- Lines of code: $LOC
- TODO remaining: $TODO
- ISO size: $ISO

## Boot Log
\`\`\`
$(cat /tmp/vxair_p5.log 2>/dev/null | head -50)
\`\`\`

## Feature Status
$(cat docs/phase5_status.md 2>/dev/null)

## How To Run
\`\`\`bash
# QEMU with GUI
bash scripts/run_qemu_gui.sh

# Flash to USB
sudo bash scripts/flash_usb.sh /dev/sdX
\`\`\`
REPORT

    echo "Report saved to docs/phase5_report.md"
    echo ""
    echo "╔══════════════════════════════════════════╗"
    echo "║   VEXTRYN AIR v0.1.0 PHASE 5 COMPLETE   ║"
    echo "╠══════════════════════════════════════════╣"
    echo "║  Files:     $FILE_COUNT"
    echo "║  LOC:       $LOC"
    echo "║  TODOs:     $TODO"
    echo "║  ISO:       $ISO"
    echo "║  Milestones: 15/15 ✅"
    echo "╠══════════════════════════════════════════╣"
    echo "║  bash scripts/run_qemu_gui.sh            ║"
    echo "╚══════════════════════════════════════════╝"
    
    cat > scripts/run_qemu_gui.sh << 'EOF'
#!/bin/bash
qemu-system-x86_64 \
  -cdrom ~/Vextryn_Air/vextryn-air.iso \
  -m 512M -smp 4 \
  -machine q35 -cpu qemu64 \
  -device virtio-net-pci,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::8080-:80 \
  -vga virtio \
  -display gtk \
  -serial stdio \
  -no-reboot
EOF
    chmod +x scripts/run_qemu_gui.sh
fi
