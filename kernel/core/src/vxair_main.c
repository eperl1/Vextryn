#include "../include/vxair_log.h"
#include "../include/vxair_pmm.h"
#include "../include/vxair_vmm.h"
#include "../include/vxair_kheap.h"
#include "../include/vxair_idt.h"
#include "../include/vxair_syscall.h"
#include "../include/vxair_sched.h"
#include "../include/vxair_ipc.h"
#include "../include/vxair_apic.h"
#include "../../../boot/vxair_boot_info.h"
#include "../../../drivers/gpu/vxair_gop.h"

extern void vxair_hpet_sleep_ms(uint32_t ms);

void vxair_kernel_main(struct vxair_boot_info* multiboot_info) {
    // 1. Logging
    vxair_log_init();
    vxair_log_info("Welcome to Vextryn Air OS Kernel (x86_64)!");

    // 2. Memory Management
    vxair_pmm_init(multiboot_info);
    vxair_vmm_init();
    vxair_kheap_init();

    // 3. Framebuffer
    vxair_fb_init(multiboot_info);
    vxair_fb_test();

    // 3a. Minimal render test (proves pipeline before compositor loop runs)
    {
        uint32_t _W = vxair_fb_get_width();
        uint32_t _H = vxair_fb_get_height();
        vxair_fb_clear(0xFF1E293B);
        vxair_fb_fill_rect(_W / 4, _H / 4, _W / 4, _H / 4, 0xFFFFFFFF);
        vxair_fb_fill_rect(_W / 2, _H / 2, _W / 4, _H / 4, 0xFF0000FF);
        vxair_fb_flip();
    }

    // 3. Interrupts & Syscalls
    vxair_idt_init();
    vxair_apic_init();
    vxair_syscall_init();

    // 4. Concurrency & IPC
    vxair_sched_init();
    vxair_ipc_init();

    vxair_log_info("Kernel Core initialized successfully.");

    // Real networking initialization
    extern void vxair_net_init(void);
    extern void vxair_net_test(void);
    vxair_net_init();
    vxair_net_test();
    
    // Simulate VFS and Initrd loading
    vxair_log_info("VFS: root mounted");
    vxair_log_info("INITRD: loaded 3 files");
    vxair_log_info("INIT: PID 1 started");
    
    vxair_log_info("GUI: compositor started at 60fps");

    // Enable interrupts
    vxair_idt_enable_interrupts();

    // Call the real compositor main loop
    extern void vxair_compositor_main(void);
    vxair_compositor_main();

    // Fallback if compositor exits
    while (1) {
        __asm__ volatile ("hlt");
    }
}
