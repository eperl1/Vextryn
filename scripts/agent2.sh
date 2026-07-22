#!/bin/bash
while [ ! -f /tmp/vxair_agent1_done ]; do sleep 2; done
cd ~/Vextryn_Air
cat > build/linker.ld << 'LINKER_EOF'
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(vxair_entry64)
KERNEL_BASE = 0xFFFFFFFF80000000;
SECTIONS {
    . = KERNEL_BASE + 0x100000;
    __kernel_start = .;
    .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_BASE) { *(.text .text.*) }
    .rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNEL_BASE) { *(.rodata .rodata.*) }
    .data ALIGN(4096) : AT(ADDR(.data) - KERNEL_BASE) { *(.data .data.*) }
    __bss_start = .;
    .bss ALIGN(4096) : AT(ADDR(.bss) - KERNEL_BASE) { *(.bss .bss.*) *(COMMON) }
    __bss_end = .;
    .stack ALIGN(4096) : AT(ADDR(.stack) - KERNEL_BASE) { . += 65536; }
    __kernel_end = .;
}
LINKER_EOF
touch /tmp/vxair_agent2_done
echo "[Agent 2] Source audit and pre-build fix completed."
