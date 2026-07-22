#ifndef VXAIR_DRIVERS_WIRELESS_RTL8852_H
#define VXAIR_DRIVERS_WIRELESS_RTL8852_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file rtl8852.h
 * @brief Realtek RTL8852 (WiFi 6) driver for Vextryn Air OS.
 */

#define VXAIR_RTL8852_PCI_VENDOR_ID 0x10EC
#define VXAIR_RTL8852_PCI_DEVICE_ID 0x8852

/**
 * @brief Buffer descriptor for RTL DMA ring.
 */
typedef struct {
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t cmd_len;
    uint32_t status;
} vxair_rtl8852_desc_t;

/**
 * @brief RTL8852 Device Context.
 */
typedef struct {
    uint8_t pci_bus;
    uint8_t pci_slot;
    uint8_t pci_func;
    
    void *mmio_base;
    size_t mmio_size;
    
    bool is_initialized;
    
    vxair_rtl8852_desc_t *tx_ring;
    uint32_t tx_ring_size;
    uint32_t tx_idx;
    
    vxair_rtl8852_desc_t *rx_ring;
    uint32_t rx_ring_size;
    uint32_t rx_idx;
    
    uint8_t mac_address[6];
} vxair_rtl8852_device_t;

/**
 * @brief Initialize the RTL8852 device.
 * @param dev Device context.
 * @param bus PCI bus number.
 * @param slot PCI slot number.
 * @param func PCI function number.
 * @return 0 on success, negative error code on failure.
 */
int vxair_rtl8852_init(vxair_rtl8852_device_t *dev, uint8_t bus, uint8_t slot, uint8_t func);

/**
 * @brief Power on and reset RTL8852 hardware.
 * @param dev Device context.
 * @return 0 on success, negative error code on failure.
 */
int vxair_rtl8852_power_on(vxair_rtl8852_device_t *dev);

/**
 * @brief Transmit packet via RTL8852.
 * @param dev Device context.
 * @param data Data pointer.
 * @param len Data length.
 * @return Number of bytes transmitted, negative error code on failure.
 */
int vxair_rtl8852_xmit(vxair_rtl8852_device_t *dev, const uint8_t *data, size_t len);

#endif /* VXAIR_DRIVERS_WIRELESS_RTL8852_H */
