#include "hal_pci.h"

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
