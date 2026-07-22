#include "hal_pm.h"
#include "hal_acpi.h"
#include <stddef.h>

// I/O port helper
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %w1, %b0" : "=a"(value) : "Nd"(port));
    return value;
}

typedef struct {
    vxair_acpi_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;
    uint16_t boot_architecture_flags;
    uint8_t reserved2;
    uint32_t flags;
} __attribute__((packed)) vxair_acpi_fadt_t;

static uint32_t g_pm1a_control_block = 0;
static uint32_t g_pm1b_control_block = 0;
static uint32_t g_smi_command_port = 0;
static uint8_t g_acpi_enable = 0;

void vxair_hal_pm_init(void) {
    vxair_acpi_fadt_t* fadt = (vxair_acpi_fadt_t*)vxair_hal_acpi_find_table("FACP");
    if (!fadt) {
        return;
    }
    
    g_pm1a_control_block = fadt->pm1a_control_block;
    g_pm1b_control_block = fadt->pm1b_control_block;
    g_smi_command_port = fadt->smi_command_port;
    g_acpi_enable = fadt->acpi_enable;
    
    // Try to enable ACPI if SMI is supported
    if (g_smi_command_port != 0 && g_acpi_enable != 0) {
        outb(g_smi_command_port, g_acpi_enable);
    }
}

void vxair_hal_pm_shutdown(void) {
    // Note: A full ACPI parser would parse the AML in the DSDT to find the \_S5 object.
    // We use common defaults for SLP_TYPa and SLP_TYPb for QEMU/Bochs.
    uint16_t slp_typa = 5; // Typical for QEMU/VirtualBox
    uint16_t slp_typb = 5; 
    
    if (g_pm1a_control_block != 0) {
        outw(g_pm1a_control_block, (slp_typa << 10) | (1 << 13));
    }
    if (g_pm1b_control_block != 0) {
        outw(g_pm1b_control_block, (slp_typb << 10) | (1 << 13));
    }
    
    // QEMU legacy fallback
    outw(0xB004, 0x2000);
    outw(0x0604, 0x2000);
    outw(0x4004, 0x3400);
    
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void vxair_hal_pm_reboot(void) {
    // Try keyboard controller reset
    uint8_t temp;
    
    // Clear keyboard buffer
    do {
        temp = inb(0x64);
        if ((temp & 0x01) != 0) {
            inb(0x60);
        }
    } while ((temp & 0x02) != 0);
    
    // Pulse reset line
    outb(0x64, 0xFE);
    
    // If we're still here, halt
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void vxair_hal_pm_suspend(void) {
    // S3 suspend requires ACPI AML parsing for \_S3 object.
    // Using common default.
    uint16_t slp_typa = 5; 
    
    if (g_pm1a_control_block != 0) {
        outw(g_pm1a_control_block, (slp_typa << 10) | (1 << 13)); // S3
    }
}
