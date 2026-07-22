#ifndef VXAIR_DRIVERS_WIRELESS_AX200_H
#define VXAIR_DRIVERS_WIRELESS_AX200_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file ax200.h
 * @brief Intel AX200 (WiFi 6) wireless driver for Vextryn Air OS.
 */

#define VXAIR_AX200_PCI_VENDOR_ID 0x8086
#define VXAIR_AX200_PCI_DEVICE_ID 0x2723

/**
 * @brief Hardware queue structure for AX200.
 */
typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t capacity;
    void *descriptors;
} vxair_ax200_queue_t;

/**
 * @brief AX200 Device Context.
 */
typedef struct {
    uint8_t pci_bus;
    uint8_t pci_slot;
    uint8_t pci_func;
    
    void *mmio_base;
    size_t mmio_size;
    
    bool is_initialized;
    bool fw_loaded;
    
    vxair_ax200_queue_t tx_queue;
    vxair_ax200_queue_t rx_queue;
    
    uint8_t mac_address[6];
} vxair_ax200_device_t;

/**
 * @brief Initialize the AX200 device.
 * @param dev Device context to initialize.
 * @param bus PCI bus number.
 * @param slot PCI slot number.
 * @param func PCI function number.
 * @return 0 on success, negative error code on failure.
 */
int vxair_ax200_init(vxair_ax200_device_t *dev, uint8_t bus, uint8_t slot, uint8_t func);

/**
 * @brief Load firmware into the AX200 device.
 * @param dev Device context.
 * @param fw_data Pointer to firmware binary.
 * @param fw_size Size of firmware binary.
 * @return 0 on success, negative error code on failure.
 */
int vxair_ax200_load_firmware(vxair_ax200_device_t *dev, const uint8_t *fw_data, size_t fw_size);

/**
 * @brief Transmit a packet via AX200.
 * @param dev Device context.
 * @param data Packet data.
 * @param length Packet length.
 * @return Number of bytes queued, or negative error code.
 */
int vxair_ax200_xmit(vxair_ax200_device_t *dev, const uint8_t *data, size_t length);

#endif /* VXAIR_DRIVERS_WIRELESS_AX200_H */
