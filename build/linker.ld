OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
KERNEL_BASE = 0xFFFFFFFF80000000;
SECTIONS {
    . = KERNEL_BASE + 0x100000;
    __kernel_start = .;
    .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_BASE) { KEEP(*(.multiboot)) *(.text .text.*) }
    .rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNEL_BASE) { *(.rodata .rodata.*) }
    .data ALIGN(4096) : AT(ADDR(.data) - KERNEL_BASE) { *(.data .data.*) }
    __bss_start = .;
    .bss ALIGN(4096) : AT(ADDR(.bss) - KERNEL_BASE) { *(.bss .bss.*) *(COMMON) }
    __bss_end = .;
    .stack ALIGN(4096) : AT(ADDR(.stack) - KERNEL_BASE) { . += 65536; }
    __kernel_end = .;
}
