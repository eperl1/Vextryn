/* 64-bit long mode entry for Vextryn Air OS */
.section .text
.global vxair_long_mode_start
.type vxair_long_mode_start, @function

.code64
vxair_long_mode_start:
    /* We are now in 64-bit long mode. */
    /* Update data segments */
    mov $0, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* Clear the frame pointer to mark the top of the stack frame */
    xor %rbp, %rbp

    /* Multiboot info pointer or UEFI boot info pointer is in %rdi */
    /* %rdi was set either by multiboot2_stub.S or uefi_bootloader.c */
    
    /* Setup higher-half stack */
    mov $stack_top, %rsp

    /* Call the early C entry point which parses boot information and populates vxair_boot_info */
    /* void vxair_early_entry(void* boot_info_ptr); */
    call vxair_early_entry

    /* The return value in %rax is a pointer to g_kernel_boot_info */
    /* Pass it to vxair_kernel_main in %rdi */
    mov %rax, %rdi

    /* Call the C/C++ kernel entry point */
    /* extern void vxair_kernel_main(struct vxair_boot_info* boot_info); */
    call vxair_kernel_main

    cli
1:  hlt
    jmp 1b

.size vxair_long_mode_start, . - vxair_long_mode_start
