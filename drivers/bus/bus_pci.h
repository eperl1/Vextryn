#ifndef VXAIR_BUS_PCI_H
#define VXAIR_BUS_PCI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Information about a discovered PCI device
 */
typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} vxair_pci_device_t;

/**
 * @brief Initialize the PCI bus driver
 */
void vxair_bus_pci_init(void);

/**
 * @brief Scan the PCI bus for devices
 */
void vxair_bus_pci_scan(void);

/**
 * @brief Get the number of discovered PCI devices
 * @return Number of devices
 */
uint32_t vxair_bus_pci_get_device_count(void);

/**
 * @brief Retrieve a specific PCI device by index
 * @param index The index of the device
 * @return Pointer to the device info, or NULL if out of bounds
 */
const vxair_pci_device_t* vxair_bus_pci_get_device(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BUS_PCI_H
