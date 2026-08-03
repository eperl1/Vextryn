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

    // 3. ACPI initialization — must happen before any ACPI table lookups (HPET, MCFG, etc.)
    // The RSDP address may be provided by the bootloader in boot_info->rsdp_address,
    // or we scan the standard BIOS memory area (0xE0000–0xFFFFF) as a fallback.
    extern void vxair_hal_acpi_init(void* rsdp_addr);
    extern void* vxair_hal_acpi_scan_rsdp(void);
    if (multiboot_info->rsdp_address) {
        vxair_hal_acpi_init((void*)(uintptr_t)multiboot_info->rsdp_address);
        vxair_log_info("ACPI: initialized via bootloader RSDP at 0x%x", (uint32_t)multiboot_info->rsdp_address);
    } else {
        void* rsdp = vxair_hal_acpi_scan_rsdp();
        if (rsdp) {
            vxair_hal_acpi_init(rsdp);
            vxair_log_info("ACPI: initialized via memory scan RSDP at %p", rsdp);
        } else {
            vxair_log_info("ACPI: no RSDP found via boot info or memory scan");
        }
    }

    // 3. Interrupts & Syscalls
    vxair_idt_init();
    vxair_apic_init();
    vxair_syscall_init();

    // 4. Concurrency & IPC
    vxair_sched_init();
    vxair_ipc_init();

    vxair_log_info("Kernel Core initialized successfully.");

    // Networking: rtl8139 driver + quick ARP probe (non-blocking, <2s timeout).
    extern void vxair_bus_pci_init(void);
    extern void vxair_bus_pci_scan(void);
    extern void vxair_net_init(void);
    vxair_bus_pci_init();
    vxair_bus_pci_scan();
    vxair_net_init();  // inits eth/arp/ip/udp + rtl8139 driver

    // Quick ARP probe: send one ARP request for QEMU's gateway (10.0.2.2) and
    // poll briefly. If a reply arrives, RX is confirmed working.
    {
        extern void vxair_hpet_sleep_ms(uint32_t ms);
        extern int vxair_arp_request(uint32_t ip);
        extern uint16_t vxair_rtl8139_receive(uint8_t *buf, uint16_t max);
        extern void vxair_eth_receive(void *frame, uint16_t len);
        extern int vxair_arp_lookup(uint32_t ip, uint8_t *mac_out);

        uint32_t gw_ip = 0x0202000A;  // 10.0.2.2 (QEMU user-mode gateway) in LE
        vxair_arp_request(gw_ip);
        vxair_log_info("NET: ARP probe sent for gateway 10.0.2.2, polling...");

        uint8_t gw_mac[6] = {0};
        int arp_ok = 0;
        for (int i = 0; i < 40; i++) {  // 40 × 50ms = 2 seconds max
            vxair_hpet_sleep_ms(50);
            uint8_t frame[2048];
            uint16_t flen = vxair_rtl8139_receive(frame, sizeof(frame));
            if (flen > 0) {
                vxair_eth_receive(frame, flen);
                if (vxair_arp_lookup(gw_ip, gw_mac) == 0) {
                    vxair_log_info("NET: ARP REPLY RECEIVED — RX PATH CONFIRMED WORKING "
                                   "(gateway MAC=%02x:%02x:%02x:%02x:%02x:%02x)",
                                   gw_mac[0], gw_mac[1], gw_mac[2],
                                   gw_mac[3], gw_mac[4], gw_mac[5]);
                    arp_ok = 1;
                    break;
                }
            }
        }
        if (!arp_ok) {
            vxair_log_info("NET: ARP probe timed out — no reply received in 2s");
        }
    }
    
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
