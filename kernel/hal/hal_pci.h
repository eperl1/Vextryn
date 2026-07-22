#ifndef VXAIR_HAL_PCI_H
#define VXAIR_HAL_PCI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/**
 * @brief Initialize the PCI HAL
 */
void vxair_hal_pci_init(void);

/**
 * @brief Read a 32-bit value from PCI configuration space
 * @param bus PCI Bus number
 * @param slot PCI Slot number
 * @param func PCI Function number
 * @param offset Register offset within the configuration space
 * @return 32-bit register value
 */
uint32_t vxair_hal_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/**
 * @brief Write a 32-bit value to PCI configuration space
 * @param bus PCI Bus number
 * @param slot PCI Slot number
 * @param func PCI Function number
 * @param offset Register offset within the configuration space
 * @param value 32-bit value to write
 */
void vxair_hal_pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

/**
 * @brief Check if a specific PCI device exists
 * @param bus PCI Bus number
 * @param slot PCI Slot number
 * @param func PCI Function number
 * @return True if the device exists, False otherwise
 */
bool vxair_hal_pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_PCI_H
