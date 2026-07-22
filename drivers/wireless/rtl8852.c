#include "rtl8852.h"

/**
 * @file rtl8852.c
 * @brief Realtek RTL8852 (WiFi 6) driver implementation.
 */

#define RTL8852_REG_SYS_CTRL     0x0000
#define RTL8852_REG_MAC_ID       0x0010
#define RTL8852_REG_TX_DESC      0x0200
#define RTL8852_REG_RX_DESC      0x0210
#define RTL8852_REG_INTR_MASK    0x0300
#define RTL8852_REG_INTR_STATUS  0x0304

#define RTL8852_CMD_OWN          0x80000000
#define RTL8852_CMD_FS           0x20000000
#define RTL8852_CMD_LS           0x10000000

static void vxair_rtl8852_write8(vxair_rtl8852_device_t *dev, uint32_t offset, uint8_t val) {
    if (dev->mmio_base) {
        volatile uint8_t *reg = (volatile uint8_t *)((uint8_t *)dev->mmio_base + offset);
        *reg = val;
    }
}

static void vxair_rtl8852_write32(vxair_rtl8852_device_t *dev, uint32_t offset, uint32_t val) {
    if (dev->mmio_base) {
        volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)dev->mmio_base + offset);
        *reg = val;
    }
}

static uint32_t vxair_rtl8852_read32(vxair_rtl8852_device_t *dev, uint32_t offset) {
    if (dev->mmio_base) {
        volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)dev->mmio_base + offset);
        return *reg;
    }
    return 0;
}

int vxair_rtl8852_power_on(vxair_rtl8852_device_t *dev) {
    if (!dev) return -1;
    
    /* Toggle sys_ctrl bits to power on MAC and PHY */
    uint32_t sys_ctrl = vxair_rtl8852_read32(dev, RTL8852_REG_SYS_CTRL);
    sys_ctrl |= 0x00000001; /* LDO power on */
    sys_ctrl |= 0x00000002; /* MAC power on */
    vxair_rtl8852_write32(dev, RTL8852_REG_SYS_CTRL, sys_ctrl);
    
    /* Dummy wait logic for hardware readiness */
    for (int i = 0; i < 1000; i++) {
        if ((vxair_rtl8852_read32(dev, RTL8852_REG_SYS_CTRL) & 0x00000100) != 0) {
            break;
        }
    }
    
    return 0;
}

int vxair_rtl8852_init(vxair_rtl8852_device_t *dev, uint8_t bus, uint8_t slot, uint8_t func) {
    if (!dev) return -1;
    
    dev->pci_bus = bus;
    dev->pci_slot = slot;
    dev->pci_func = func;
    dev->is_initialized = false;
    
    dev->mmio_base = (void *)0xE0000000;
    dev->mmio_size = 0x10000;
    
    if (vxair_rtl8852_power_on(dev) != 0) {
        return -2;
    }
    
    /* Read MAC ID */
    uint32_t mac_low = vxair_rtl8852_read32(dev, RTL8852_REG_MAC_ID);
    uint32_t mac_high = vxair_rtl8852_read32(dev, RTL8852_REG_MAC_ID + 4);
    
    dev->mac_address[0] = (mac_low >> 0) & 0xFF;
    dev->mac_address[1] = (mac_low >> 8) & 0xFF;
    dev->mac_address[2] = (mac_low >> 16) & 0xFF;
    dev->mac_address[3] = (mac_low >> 24) & 0xFF;
    dev->mac_address[4] = (mac_high >> 0) & 0xFF;
    dev->mac_address[5] = (mac_high >> 8) & 0xFF;
    
    dev->tx_ring_size = 128;
    dev->tx_ring = (vxair_rtl8852_desc_t *)0x4000000; /* Dummy alloc */
    dev->tx_idx = 0;
    
    dev->rx_ring_size = 128;
    dev->rx_ring = (vxair_rtl8852_desc_t *)0x5000000; /* Dummy alloc */
    dev->rx_idx = 0;
    
    /* Set up DMA rings in HW */
    vxair_rtl8852_write32(dev, RTL8852_REG_TX_DESC, 0x4000000);
    vxair_rtl8852_write32(dev, RTL8852_REG_RX_DESC, 0x5000000);
    
    /* Enable interrupts */
    vxair_rtl8852_write32(dev, RTL8852_REG_INTR_MASK, 0xFFFFFFFF);
    
    dev->is_initialized = true;
    return 0;
}

int vxair_rtl8852_xmit(vxair_rtl8852_device_t *dev, const uint8_t *data, size_t len) {
    if (!dev || !dev->is_initialized) return -1;
    if (len == 0 || len > 4096) return -2;
    
    uint32_t idx = dev->tx_idx;
    vxair_rtl8852_desc_t *desc = &dev->tx_ring[idx];
    
    /* Check if descriptor is owned by HW */
    if (desc->cmd_len & RTL8852_CMD_OWN) {
        return -3; /* Ring full */
    }
    
    /* In reality, we must use virt_to_phys(data) */
    desc->addr_low = (uint32_t)(uintptr_t)data;
    desc->addr_high = 0; 
    
    /* Set length, First Segment, Last Segment, and transfer OWN to HW */
    desc->cmd_len = (len & 0xFFFF) | RTL8852_CMD_FS | RTL8852_CMD_LS | RTL8852_CMD_OWN;
    
    dev->tx_idx = (idx + 1) % dev->tx_ring_size;
    
    /* Ring TX doorbell */
    vxair_rtl8852_write8(dev, 0x0250, 0x01); /* Polling demand */
    
    return len;
}
