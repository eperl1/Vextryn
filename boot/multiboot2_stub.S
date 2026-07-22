/* Multiboot2 stub for Vextryn Air OS */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0xE85250D6       /* Multiboot2 magic number */
.set ARCH,     0                /* protected mode i386 */
.set HEADER_LENGTH, header_end - header_start
.set CHECKSUM, -(MAGIC + ARCH + HEADER_LENGTH)
.set KERNEL_BASE, 0xFFFFFFFF80000000

.section .multiboot
.align 8
header_start:
    .long MAGIC
    .long ARCH
    .long HEADER_LENGTH
    .long CHECKSUM

    /* Framebuffer tag */
    .align 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

    /* End tag */
    .align 8
    .short 0
    .short 0
    .long 8
header_end:

.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB stack */
.global stack_top
stack_top:

/* Page tables for 64-bit transition */
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096

.section .rodata
gdt64:
    .quad 0 /* zero entry */
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53) /* code segment */
    .quad (1<<44) | (1<<47) /* data segment */
.align 4
gdt64_pointer:
    .short . - gdt64 - 1
    .quad gdt64 - KERNEL_BASE

.section .text
.code32
.global _start
.type _start, @function
_start:
    mov $(stack_top - KERNEL_BASE), %esp

    /* Multiboot2 info structure is passed in %ebx */
    /* Magic number is passed in %eax */
    
    /* Verify multiboot2 magic */
    cmp $0x36d76289, %eax
    jne no_multiboot

    /* Store multiboot pointer for later */
    mov %ebx, %edi

    /* Setup paging for 64-bit long mode (PML4, PDPT, PD) */
    /* Map first 1GB to 2MB pages */
    mov $(p3_table - KERNEL_BASE), %eax
    or $0b11, %eax /* present + writable */
    mov %eax, (p4_table - KERNEL_BASE)
    mov %eax, (p4_table - KERNEL_BASE + 511*8) /* Map higher half P4[511] */

    mov $(p2_table - KERNEL_BASE), %eax
    or $0b11, %eax
    mov %eax, (p3_table - KERNEL_BASE)
    mov %eax, (p3_table - KERNEL_BASE + 510*8) /* Map higher half P3[510] */


    mov $0, %ecx
map_p2_table:
    /* map a 2MiB huge page */
    mov $0x200000, %eax
    mul %ecx
    or $0b10000011, %eax /* present + writable + huge */
    mov %eax, (p2_table - KERNEL_BASE)(,%ecx,8)
    
    inc %ecx
    cmp $512, %ecx
    jne map_p2_table

    /* Load P4 to CR3 */
    mov $(p4_table - KERNEL_BASE), %eax
    mov %eax, %cr3

    /* Enable PAE */
    mov %cr4, %eax
    or $1 << 5, %eax
    mov %eax, %cr4

    /* Set the long mode bit in the EFER MSR (model specific register) */
    mov $0xC0000080, %ecx
    rdmsr
    or $1 << 8, %eax
    wrmsr

    /* Enable paging */
    mov %cr0, %eax
    or $1 << 31, %eax
    mov %eax, %cr0

    /* Load GDT with 64-bit code segment */
    lgdt (gdt64_pointer - KERNEL_BASE)

    /* Jump to long mode entry point (vxair_long_mode_start) */
    /* We use a far jump to the 64-bit code segment */
    ljmp $8, $(vxair_long_mode_start - KERNEL_BASE)

no_multiboot:
    /* Loop if not booted by multiboot */
    cli
1:  hlt
    jmp 1b

.size _start, . - _start

