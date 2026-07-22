#include "hal_timer.h"
#include "hal_acpi.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    vxair_acpi_header_t header;
    uint32_t hardware_rev_id;
    uint32_t comparator_count:5;
    uint32_t counter_size:1;
    uint32_t reserved:1;
    uint32_t legacy_replacement:1;
    uint32_t pci_vendor_id:16;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved2;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) vxair_acpi_hpet_t;

static volatile uint64_t* g_hpet_regs = NULL;
static uint64_t g_hpet_period = 0; // in femtoseconds
static bool g_hpet_available = false;

void vxair_hal_timer_init(void) {
    vxair_acpi_hpet_t* hpet_table = (vxair_acpi_hpet_t*)vxair_hal_acpi_find_table("HPET");
    if (!hpet_table) {
        g_hpet_available = false;
        return;
    }
    
    g_hpet_regs = (volatile uint64_t*)(uintptr_t)hpet_table->address;
    
    uint64_t capabilities = g_hpet_regs[0];
    g_hpet_period = capabilities >> 32;
    
    // Enable main counter
    uint64_t config = g_hpet_regs[2];
    config |= 1; // ENABLE_CNF
    g_hpet_regs[2] = config;
    
    g_hpet_available = true;
}

uint64_t vxair_hal_timer_get_uptime_ms(void) {
    if (!g_hpet_available || !g_hpet_period) {
        return 0; // Fallback required in full implementation
    }
    
    uint64_t main_counter = g_hpet_regs[30]; // 0xF0 offset / 8 = 30
    // femtoseconds to milliseconds: divide by 10^12
    return (main_counter * g_hpet_period) / 1000000000000ULL;
}

void vxair_hal_timer_sleep_ms(uint64_t ms) {
    if (!g_hpet_available) {
        return; // Fallback delay loop would go here
    }
    
    uint64_t start = vxair_hal_timer_get_uptime_ms();
    while (vxair_hal_timer_get_uptime_ms() - start < ms) {
        // Busy wait
        __asm__ volatile ("pause" ::: "memory");
    }
}
