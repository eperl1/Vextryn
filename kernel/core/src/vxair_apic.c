#include "../include/vxair_apic.h"
#include "../include/vxair_log.h"
#include "../include/vxair_vmm.h"
#include "../include/vxair_pmm.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800

static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static volatile uint32_t* lapic_base;

static inline uint32_t lapic_read(uint32_t reg) {
    return lapic_base[reg / 4];
}

static inline void lapic_write(uint32_t reg, uint32_t val) {
    lapic_base[reg / 4] = val;
}

extern void* kernel_pml4;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void vxair_apic_init(void) {
    vxair_log_info("APIC: Initializing...");
    
    // Disable legacy PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    uint64_t apic_msr = rdmsr(IA32_APIC_BASE_MSR);
    vxair_paddr_t apic_paddr = apic_msr & 0xFFFFFFFFFFFFF000;
    
    // Map APIC base
    vxair_vmm_map_page(kernel_pml4, apic_paddr, apic_paddr, VXAIR_VMM_PRESENT | VXAIR_VMM_RW);
    
    lapic_base = (volatile uint32_t*)apic_paddr;
    
    wrmsr(IA32_APIC_BASE_MSR, apic_msr | IA32_APIC_BASE_MSR_ENABLE);
    lapic_write(0xF0, lapic_read(0xF0) | 0x100 | 0xFF);
    
    // Setup APIC timer
    lapic_write(0x3E0, 0x3); // Divide by 16
    lapic_write(0x320, 32 | 0x20000); // Periodic, vector 32
    lapic_write(0x380, 1000000); // Initial count
    
    vxair_log_info("APIC: initialized at base %p", (void*)apic_paddr);
}

void vxair_apic_eoi(void) {
    lapic_write(0xB0, 0);
}
