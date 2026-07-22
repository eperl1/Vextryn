#include "bus_pci.h"
#include "../../kernel/hal/hal_pci.h"
#include <stddef.h>

#define MAX_PCI_DEVICES 256

static vxair_pci_device_t g_pci_devices[MAX_PCI_DEVICES];
static uint32_t g_pci_device_count = 0;

void vxair_bus_pci_init(void) {
    g_pci_device_count = 0;
    vxair_hal_pci_init();
}

static void vxair_bus_pci_check_device(uint8_t bus, uint8_t slot, uint8_t func) {
    if (g_pci_device_count >= MAX_PCI_DEVICES) {
        return; // Device array full
    }

    if (!vxair_hal_pci_device_exists(bus, slot, func)) {
        return;
    }

    uint32_t id_reg = vxair_hal_pci_read_config(bus, slot, func, 0x00);
    uint32_t class_reg = vxair_hal_pci_read_config(bus, slot, func, 0x08);
    
    vxair_pci_device_t* dev = &g_pci_devices[g_pci_device_count++];
    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    dev->vendor_id = id_reg & 0xFFFF;
    dev->device_id = (id_reg >> 16) & 0xFFFF;
    dev->class_code = (class_reg >> 24) & 0xFF;
    dev->subclass = (class_reg >> 16) & 0xFF;
    dev->prog_if = (class_reg >> 8) & 0xFF;
}

void vxair_bus_pci_scan(void) {
    g_pci_device_count = 0;
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            if (vxair_hal_pci_device_exists(bus, slot, 0)) {
                vxair_bus_pci_check_device(bus, slot, 0);
                
                // Check if multi-function
                uint32_t header_type = vxair_hal_pci_read_config(bus, slot, 0, 0x0C);
                if ((header_type >> 16) & 0x80) {
                    for (uint8_t func = 1; func < 8; func++) {
                        vxair_bus_pci_check_device(bus, slot, func);
                    }
                }
            }
        }
    }
}

uint32_t vxair_bus_pci_get_device_count(void) {
    return g_pci_device_count;
}

const vxair_pci_device_t* vxair_bus_pci_get_device(uint32_t index) {
    if (index < g_pci_device_count) {
        return &g_pci_devices[index];
    }
    return NULL;
}
