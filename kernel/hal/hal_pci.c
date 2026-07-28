#include "hal_pci.h"
#include "hal_acpi.h"
#include "../core/include/vxair_vmm.h"
#include <string.h>
#include <stddef.h>

extern void vxair_log_info(const char* fmt, ...);
extern vxair_page_table_t* kernel_pml4;

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void vxair_hal_pci_init(void) {
    // PCI subsystem initialization (legacy PCI)
}

uint32_t vxair_hal_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    
    // Create configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
              
    // Write out the address
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read the data
    return inl(PCI_CONFIG_DATA);
}

void vxair_hal_pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
              
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

bool vxair_hal_pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t vendor_id = vxair_hal_pci_read_config(bus, slot, func, 0) & 0xFFFF;
    return (vendor_id != 0xFFFF);
}

// ===== PCIe MMCONFIG (Extended Config Space) via ACPI MCFG =====

typedef struct {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint8_t  reserved[8];
} vxair_acpi_mcfg_t;

typedef struct {
    uint64_t base_address;
    uint16_t pci_segment;
    uint8_t  start_bus;
    uint8_t  end_bus;
    uint32_t reserved;
} vxair_acpi_mcfg_entry_t;

static uint64_t g_mmcfg_base     = 0;
static uint8_t  g_mmcfg_startbus = 0;
static bool     g_mmcfg_ready    = false;

bool vxair_hal_pci_mmconfig_init(void) {
    void* table = vxair_hal_acpi_find_table("MCFG");
    if (!table) {
        vxair_log_info("PCIE: MCFG table not found — ACPI not initialized or not present");
        return false;
    }
    vxair_acpi_mcfg_t* mcfg = (vxair_acpi_mcfg_t*)table;
    uint32_t total_len = mcfg->length;
    uint32_t header_size = sizeof(vxair_acpi_mcfg_t);
    if (total_len <= header_size) {
        vxair_log_info("PCIE: MCFG table too short (%u bytes)", total_len);
        return false;
    }
    uint32_t num_entries = (total_len - header_size) / sizeof(vxair_acpi_mcfg_entry_t);
    vxair_log_info("PCIE: MCFG found, %u entries", num_entries);
    vxair_acpi_mcfg_entry_t* entries = (vxair_acpi_mcfg_entry_t*)((uint8_t*)mcfg + header_size);
    for (uint32_t i = 0; i < num_entries; i++) {
        vxair_log_info("PCIE: MCFG entry %u: base=0x%llx seg=%u bus=%u..%u",
                       i,
                       (unsigned long long)entries[i].base_address,
                       entries[i].pci_segment,
                       entries[i].start_bus,
                       entries[i].end_bus);
        if (entries[i].pci_segment == 0 &&
            (uint32_t)entries[i].start_bus <= 0 && 0 <= (uint32_t)entries[i].end_bus) {
            g_mmcfg_base     = entries[i].base_address;
            g_mmcfg_startbus = entries[i].start_bus;
            g_mmcfg_ready    = true;
            // Identity-map MMCONFIG region with NOCACHE (MMIO must be uncacheable).
            // Map enough for bus 0 (1 MB) to cover all devices on the root bus.
            uint64_t mmcfg_map_size = 0x100000; // 1 MB covers bus 0 entirely
            vxair_log_info("PCIE: mapping MMCONFIG at 0x%llx size=0x%llx (flags=PRESENT|RW|NOCACHE)",
                           (unsigned long long)g_mmcfg_base, (unsigned long long)mmcfg_map_size);
            for (uint64_t p = g_mmcfg_base; p < g_mmcfg_base + mmcfg_map_size; p += 0x1000) {
                vxair_vmm_map_page(kernel_pml4, p, p,
                                   VXAIR_VMM_PRESENT | VXAIR_VMM_RW | VXAIR_VMM_NOCACHE);
            }
            return true;
        }
    }
    vxair_log_info("PCIE: No suitable MCFG entry found (segment 0)");
    return false;
}

static inline uint64_t mmcfg_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // PCIe MMCONFIG address: base + (bus << 20) | (slot << 15) | (func << 12) | offset
    // Note: use ONLY + (not |) for correctness — the base may have bits set in lower 20 bits
    // if the MCFG region is not 1 MB-aligned (paranoid, but safe). The shifted values are
    // guaranteed non-overlapping in bits 20+ / 15-19 / 12-14 / 0-11.
    return g_mmcfg_base
         + ((uint64_t)((uint32_t)bus - (uint32_t)g_mmcfg_startbus) << 20)
         + ((uint64_t)(slot & 0x1F) << 15)
         + ((uint64_t)(func & 0x7) << 12)
         + (offset & 0xFFF);
}

bool vxair_hal_pci_mmconfig_is_ready(void) {
    return g_mmcfg_ready;
}

uint64_t vxair_hal_pci_mmconfig_calc_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return mmcfg_addr(bus, slot, func, offset);
}

void vxair_hal_pci_write_config_mmconfig(uint8_t bus, uint8_t slot, uint8_t func,
                                         uint8_t offset, uint32_t value) {
    if (!g_mmcfg_ready) return;
    volatile uint32_t* ptr = (volatile uint32_t*)(uintptr_t)mmcfg_addr(bus, slot, func, offset);
    *ptr = value;
}

uint32_t vxair_hal_pci_read_config_mmconfig(uint8_t bus, uint8_t slot, uint8_t func,
                                            uint8_t offset) {
    if (!g_mmcfg_ready) return 0xFFFFFFFFu;
    volatile uint32_t* ptr = (volatile uint32_t*)(uintptr_t)mmcfg_addr(bus, slot, func, offset);
    return *ptr;
}
