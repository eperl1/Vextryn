#include "e1000.h"
#include "../../drivers/bus/bus_pci.h"
#include "../../kernel/hal/hal_pci.h"
#include "../../kernel/core/include/vxair_pmm.h"
#include "../../kernel/core/include/vxair_vmm.h"
#include <string.h>
#include <stddef.h>

extern void vxair_log_info(const char *fmt, ...);
extern vxair_page_table_t *kernel_pml4;

// State
vxair_e1000_t g_e1000;

// Identity-mapped physical-to-virtual (kernel identity-maps first 4GB)
static inline void* phys_to_virt(uint64_t paddr) {
    return (void*)(uintptr_t)paddr;
}

// MMIO helpers — all registers are 32-bit except where noted
static inline uint32_t mmio_read32(volatile uint8_t *mmio, uint32_t off) {
    return *(volatile uint32_t *)(mmio + off);
}
static inline void mmio_write32(volatile uint8_t *mmio, uint32_t off, uint32_t val) {
    *(volatile uint32_t *)(mmio + off) = val;
}
static inline uint16_t mmio_read16(volatile uint8_t *mmio, uint32_t off) {
    return *(volatile uint16_t *)(mmio + off);
}

// Re-assert Bus Master + Memory Space through both legacy CF8/CFC and PCIe MMCONFIG,
// log the value, and re-read. Used as a defensive measure before any TX/RX.
static void e1000_pci_command_ensure(void) {
    if (!g_e1000.found) return;
    uint8_t bus  = g_e1000.pci_bus;
    uint8_t slot = g_e1000.pci_slot;
    uint8_t func = g_e1000.pci_func;
    uint32_t cmd = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("E1000: PCI CMD before TX/RX: 0x%x", cmd);
    cmd |= (1 << 2) | (1 << 1);
    vxair_hal_pci_write_config(bus, slot, func, 0x04, cmd);
    vxair_hal_pci_write_config_mmconfig(bus, slot, func, 0x04, cmd);
    uint32_t cmd_after = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("E1000: PCI CMD after TX/RX: 0x%x", cmd_after);
}

int vxair_e1000_init(void) {
    memset(&g_e1000, 0, sizeof(g_e1000));

    // ---- Find e1000 PCI device ----
    uint32_t dev_count = vxair_bus_pci_get_device_count();
    uint8_t bus = 0, slot = 0, func = 0;
    int found = 0;
    for (uint32_t i = 0; i < dev_count; i++) {
        const vxair_pci_device_t *dev = vxair_bus_pci_get_device(i);
        if (dev->vendor_id == 0x8086) {
            // Check for e1000 family device IDs
            if (dev->device_id == 0x100E || dev->device_id == 0x100F ||
                dev->device_id == 0x10D3 || dev->device_id == 0x1010 ||
                dev->device_id == 0x1012 || dev->device_id == 0x101D) {
            bus = dev->bus; slot = dev->slot; func = dev->func;
            found = 1;
            vxair_log_info("E1000: Found device 0x%x at bus=%u slot=%u",
                           dev->device_id, bus, slot);
            g_e1000.pci_bus = bus;
            g_e1000.pci_slot = slot;
            g_e1000.pci_func = func;
            break;
            }
        }
    }
    if (!found) {
        vxair_log_info("E1000: No e1000 device found");
        return -1;
    }

    // ---- Read BAR0 (MMIO) ----
    uint32_t bar0 = vxair_hal_pci_read_config(bus, slot, func, 0x10);
    if (bar0 & 1) {
        vxair_log_info("E1000: BAR0 is I/O (expected MMIO)");
        return -1;
    }
    // MMIO BAR: bits 31-0, with type in bits 2-1
    // For 32-bit MMIO BAR: clear lower 4 bits (flags)
    // For 64-bit MMIO BAR: clear lower 4 bits, combine with BAR1
    uint64_t mmio_paddr;
    uint8_t bar_type = (bar0 >> 1) & 3;
    if (bar_type == 0) {
        // 32-bit MMIO BAR
        mmio_paddr = bar0 & 0xFFFFFFF0u;
    } else if (bar_type == 2) {
        // 64-bit MMIO BAR
        uint32_t bar1 = vxair_hal_pci_read_config(bus, slot, func, 0x14);
        mmio_paddr = ((uint64_t)bar0 & 0xFFFFFFF0u) | ((uint64_t)bar1 << 32);
    } else {
        vxair_log_info("E1000: Unknown BAR type %u", bar_type);
        return -1;
    }

    g_e1000.mmio_paddr = mmio_paddr;
    vxair_log_info("E1000: MMIO base = 0x%llx", (unsigned long long)mmio_paddr);

    // Identity-map the MMIO region in kernel page tables (same pattern as APIC driver).
    // e1000 registers span offsets 0x0000-0x5404 (RAL/RAH): need pages 0-5 (24KB)
    for (uint64_t page = mmio_paddr; page < mmio_paddr + 0x6000; page += 0x1000) {
        if (!vxair_vmm_map_page(kernel_pml4, page, page,
                                VXAIR_VMM_PRESENT | VXAIR_VMM_RW)) {
            vxair_log_info("E1000: Failed to map MMIO page 0x%llx", (unsigned long long)page);
            return -1;
        }
    }

    // Initialize PCIe MMCONFIG (parses ACPI MCFG) so we can also write the
    // command register through the PCIe extended config space.
    vxair_hal_pci_mmconfig_init();

    // ---- Enable bus master + memory space ----
    uint32_t cmd = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("E1000: PCI CMD before write: 0x%x", cmd);
    cmd |= (1 << 2) | (1 << 1);  // Bus Master Enable | Memory Space Enable
    vxair_hal_pci_write_config(bus, slot, func, 0x04, cmd);
    vxair_hal_pci_write_config_mmconfig(bus, slot, func, 0x04, cmd);
    uint32_t cmd_after = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("E1000: PCI CMD after MMCONFIG write: 0x%x", cmd_after);

    // ---- Identity-map MMIO ----
    volatile uint8_t *mmio = (volatile uint8_t *)phys_to_virt(mmio_paddr);
    g_e1000.mmio = mmio;

    // QEMU leaves the 82540EM in a usable state with the link up; do not
    // reset the device. Read the MAC from the receive address registers
    // (loaded from EEPROM by the model).
    uint32_t ral = mmio_read32(mmio, E1000_RAL);
    uint32_t rah = mmio_read32(mmio, E1000_RAH);

    // Ensure Set-Link-Up and Full-Duplex are asserted (some QEMU builds
    // leave the link down until SLU is written).
    uint32_t ctrl = mmio_read32(mmio, E1000_CTRL);
    mmio_write32(mmio, E1000_CTRL, ctrl | E1000_CTRL_SLU | E1000_CTRL_FD);
    vxair_log_info("E1000: CTRL orig=0x%x set=0x%x rb=0x%x", ctrl,
                   ctrl | E1000_CTRL_SLU | E1000_CTRL_FD,
                   mmio_read32(mmio, E1000_CTRL));
    uint8_t mac[6];
    mac[0] = (uint8_t)(ral);
    mac[1] = (uint8_t)(ral >> 8);
    mac[2] = (uint8_t)(ral >> 16);
    mac[3] = (uint8_t)(ral >> 24);
    mac[4] = (uint8_t)(rah);
    mac[5] = (uint8_t)(rah >> 8);
    memcpy(g_e1000.mac_addr, mac, 6);
    vxair_log_info("E1000: MAC=%02x:%02x:%02x:%02x:%02x:%02x (RAL=0x%x RAH=0x%x)",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ral, rah);

    // ---- Allocate descriptor rings via PMM ----
    // Each ring: RING_SIZE * 16 bytes, page-aligned
    size_t ring_bytes = E1000_TX_RING_SIZE * sizeof(e1000_tx_desc_t);
    g_e1000.tx_desc_paddr = vxair_pmm_alloc_page();  // 4KB covers 16 descriptors easily
    g_e1000.rx_desc_paddr = vxair_pmm_alloc_page();
    g_e1000.tx_desc = (e1000_tx_desc_t *)phys_to_virt(g_e1000.tx_desc_paddr);
    g_e1000.rx_desc = (e1000_rx_desc_t *)phys_to_virt(g_e1000.rx_desc_paddr);
    memset(g_e1000.tx_desc, 0, E1000_TX_RING_SIZE * sizeof(e1000_tx_desc_t));
    memset(g_e1000.rx_desc, 0, E1000_RX_RING_SIZE * sizeof(e1000_rx_desc_t));

    // ---- Allocate packet buffers via PMM ----
    // TX buffers: RING_SIZE * BUFFER_SIZE bytes
    size_t tx_bufs_bytes = E1000_TX_RING_SIZE * E1000_BUFFER_SIZE;
    size_t tx_bufs_pages = (tx_bufs_bytes + 4095) / 4096;
    g_e1000.tx_bufs_paddr = vxair_pmm_alloc_pages(tx_bufs_pages);
    g_e1000.tx_bufs = (uint8_t *)phys_to_virt(g_e1000.tx_bufs_paddr);

    // RX buffers: RING_SIZE * BUFFER_SIZE bytes
    size_t rx_bufs_bytes = E1000_RX_RING_SIZE * E1000_BUFFER_SIZE;
    size_t rx_bufs_pages = (rx_bufs_bytes + 4095) / 4096;
    g_e1000.rx_bufs_paddr = vxair_pmm_alloc_pages(rx_bufs_pages);
    g_e1000.rx_bufs = (uint8_t *)phys_to_virt(g_e1000.rx_bufs_paddr);

    // ---- Verify RX descriptor struct layout against Intel 82540EM datasheet ----
    // Datasheet legacy RX descriptor layout:
    //   uint64_t addr    (offset 0)
    //   uint16_t length  (offset 8)
    //   uint16_t checksum (offset 10)
    //   uint8_t  status  (offset 12)
    //   uint8_t  errors  (offset 13)
    //   uint16_t special (offset 14)
    // Total: 16 bytes, packed.
    vxair_log_info("E1000: sizeof(e1000_rx_desc_t)=%u expected=16", sizeof(e1000_rx_desc_t));
    vxair_log_info("E1000: RX desc offsets: addr=%u len=%u csum=%u status=%u errors=%u special=%u",
                   offsetof(e1000_rx_desc_t, addr),
                   offsetof(e1000_rx_desc_t, length),
                   offsetof(e1000_rx_desc_t, checksum),
                   offsetof(e1000_rx_desc_t, status),
                   offsetof(e1000_rx_desc_t, errors),
                   offsetof(e1000_rx_desc_t, special));

    // ---- TX setup (reference order) ----
    mmio_write32(mmio, E1000_TDBAL, (uint32_t)(g_e1000.tx_desc_paddr & 0xFFFFFFFF));
    mmio_write32(mmio, E1000_TDBAH, (uint32_t)(g_e1000.tx_desc_paddr >> 32));
    mmio_write32(mmio, E1000_TDLEN, E1000_TX_RING_SIZE * sizeof(e1000_tx_desc_t));
    mmio_write32(mmio, E1000_TDH, 0);
    mmio_write32(mmio, E1000_TDT, 0);
    mmio_write32(mmio, E1000_TIPG, E1000_TIPG_DEFAULT);
    mmio_write32(mmio, E1000_TCTRL,
                 E1000_TCTRL_EN | E1000_TCTRL_PSP | E1000_TCTRL_CT | E1000_TCTRL_COLD);

    // ---- RX setup (reference order: RAL/RAH, MTA, ring, RCTL) ----
    // Set receive address registers before enabling RX.
    uint32_t ral_w = (uint32_t)mac[0] | ((uint32_t)mac[1] << 8) |
                     ((uint32_t)mac[2] << 16) | ((uint32_t)mac[3] << 24);
    uint32_t rah_w = ((uint32_t)mac[4] | ((uint32_t)mac[5] << 8)) | E1000_RAH_AV;
    mmio_write32(mmio, E1000_RAL, ral_w);
    mmio_write32(mmio, E1000_RAH, rah_w);
    vxair_log_info("E1000: RAL=0x%x RAH=0x%x", ral_w, rah_w);

    // Clear the multicast table array (128 bytes).
    for (int i = 0; i < 128; i += 4)
        mmio_write32(mmio, E1000_MTA + i, 0);

    // Disable interrupts (polling mode).
    mmio_write32(mmio, E1000_IMS, 0);

    // Setup RX descriptors.
    for (int i = 0; i < E1000_RX_RING_SIZE; i++) {
        g_e1000.rx_desc[i].addr = g_e1000.rx_bufs_paddr + (i * E1000_BUFFER_SIZE);
        g_e1000.rx_desc[i].status = 0;
    }

    // Write RX ring registers.
    mmio_write32(mmio, E1000_RDBAL, (uint32_t)(g_e1000.rx_desc_paddr & 0xFFFFFFFF));
    mmio_write32(mmio, E1000_RDBAH, (uint32_t)(g_e1000.rx_desc_paddr >> 32));
    mmio_write32(mmio, E1000_RDLEN, E1000_RX_RING_SIZE * sizeof(e1000_rx_desc_t));
    mmio_write32(mmio, E1000_RDH, 0);
    mmio_write32(mmio, E1000_RDT, E1000_RX_RING_SIZE - 1);  // All descriptors available
    g_e1000.rx_tail = 0;

    // Read back RX ring registers to confirm.
    vxair_log_info("E1000: RX ring: RDBAL=0x%x RDBAH=0x%x RDLEN=%u RDH=%u RDT=%u",
                   mmio_read32(mmio, E1000_RDBAL),
                   mmio_read32(mmio, E1000_RDBAH),
                   mmio_read32(mmio, E1000_RDLEN),
                   mmio_read16(mmio, E1000_RDH),
                   mmio_read16(mmio, E1000_RDT));

    // Enable RX. Use the reference bits plus promiscuous flags so we
    // accept any unicast/multicast/broadcast frame while debugging.
    uint32_t rctl = mmio_read32(mmio, E1000_RCTRL);
    uint32_t rctl_orig = rctl;
    rctl |= E1000_RCTRL_EN | E1000_RCTRL_BAM | E1000_RCTRL_SECRC |
            E1000_RCTRL_UPE | E1000_RCTRL_MPE | E1000_RCTRL_SBP;
    rctl &= ~E1000_RCTRL_LBM_MASK;
    mmio_write32(mmio, E1000_RCTRL, rctl);
    uint32_t rctl_rb = mmio_read32(mmio, E1000_RCTRL);
    vxair_log_info("E1000: RCTL orig=0x%x set=0x%x readback=0x%x (EN=%d BAM=%d SECRC=%d UPE=%d MPE=%d SBP=%d)",
                   rctl_orig, rctl, rctl_rb,
                   (rctl_rb >> 1) & 1, (rctl_rb >> 15) & 1,
                   (rctl_rb >> 26) & 1, (rctl_rb >> 3) & 1,
                   (rctl_rb >> 4) & 1, (rctl_rb >> 2) & 1);

    g_e1000.found = true;
    vxair_log_info("E1000: Init OK (TX=%u RX=%u)",
                   E1000_TX_RING_SIZE, E1000_RX_RING_SIZE);

    vxair_e1000_loopback_test();
    return 0;
}

int vxair_e1000_send(const uint8_t *data, uint16_t len) {
    if (!g_e1000.found || !g_e1000.mmio) return -1;
    if (len > E1000_BUFFER_SIZE) return -1;

    // Re-assert Bus Master + Memory Space via both legacy and MMCONFIG,
    // then read back to confirm it stuck.
    e1000_pci_command_ensure();

    volatile uint8_t *mmio = g_e1000.mmio;
    uint16_t idx = g_e1000.tx_tail;

    // Copy packet to TX buffer
    memcpy(g_e1000.tx_bufs + idx * E1000_BUFFER_SIZE, data, len);
    // Pad to minimum Ethernet frame size (60 bytes payload + 4-byte FCS = 64 on wire)
    if (len < 60) {
        memset(g_e1000.tx_bufs + idx * E1000_BUFFER_SIZE + len, 0, 60 - len);
        len = 60;
    }

    // Fill descriptor
    e1000_tx_desc_t *desc = &g_e1000.tx_desc[idx];
    desc->addr   = g_e1000.tx_bufs_paddr + (idx * E1000_BUFFER_SIZE);
    desc->length = len;
    desc->cso    = 0;
    desc->cmd    = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS | E1000_TXD_CMD_IFCS;
    desc->status = 0;
    desc->css    = 0;
    desc->special = 0;

    // Advance TDT — e1000 polls descriptors continuously, no explicit notification
    g_e1000.tx_tail = (idx + 1) % E1000_TX_RING_SIZE;
    __asm__ volatile("mfence" ::: "memory");
    mmio_write32(mmio, E1000_TDT, g_e1000.tx_tail);

    vxair_log_info("E1000: TX posted idx=%u len=%u addr=0x%llx",
                   idx, len, (unsigned long long)desc->addr);

    // Poll for completion — hardware sets DD bit in status when done
    for (int p = 0; p < 5000; p++) {
        __asm__ volatile("mfence" ::: "memory");
        if (desc->status & E1000_TXD_STAT_DD) {
            vxair_log_info("E1000: TX done idx=%u iterations=%d", idx, p);
            return 0;
        }
        for (volatile int d = 0; d < 100; d++) __asm__ volatile("pause");
    }

    vxair_log_info("E1000: TX timeout idx=%u status=0x%x cmd=0x%x",
                   idx, desc->status, desc->cmd);
    return -1;
}

uint16_t vxair_e1000_receive(uint8_t *out_buf, uint16_t max_len) {
    if (!g_e1000.found || !g_e1000.mmio || !out_buf) return 0;

    // Re-assert Bus Master + Memory Space via both legacy and MMCONFIG,
    // then read back to confirm it stuck.
    e1000_pci_command_ensure();

    volatile uint8_t *mmio = g_e1000.mmio;

    // Diagnostic: check RDH vs our rx_tail
    uint16_t rdh = mmio_read16(mmio, E1000_RDH);
    uint16_t idx = g_e1000.rx_tail;

    __asm__ volatile("mfence" ::: "memory");
    // Use a volatile pointer so the compiler never caches the status byte
    // (the hardware may update it via DMA at any time).
    volatile e1000_rx_desc_t *desc = (volatile e1000_rx_desc_t *)&g_e1000.rx_desc[idx];

    // Read memory barrier to ensure we see the latest DMA write to descriptor status.
    __asm__ volatile("lfence" ::: "memory");
    uint8_t status = desc->status;

    // Check if descriptor is done
    if (!(status & E1000_RXD_STAT_DD)) {
        // Diagnostic: log status of first 4 descriptors and RDH every 16th call
        static int rx_miss_count = 0;
        if (++rx_miss_count == 1 || rx_miss_count == 16) {
            vxair_log_info("E1000: RX miss #%d RDH=%u rxtail=%u d0_st=0x%x d1_st=0x%x d2_st=0x%x d3_st=0x%x",
                           rx_miss_count, rdh, idx,
                           g_e1000.rx_desc[0].status, g_e1000.rx_desc[1].status,
                           g_e1000.rx_desc[2].status, g_e1000.rx_desc[3].status);
        }
        return 0;
    }

    uint16_t frame_len = desc->length;
    if (frame_len > max_len) frame_len = max_len;
    if (frame_len > 0) {
        memcpy(out_buf, g_e1000.rx_bufs + idx * E1000_BUFFER_SIZE, frame_len);
    }

    // Recycle descriptor: reset status, advance tail, update RDT.
    // Write RDT = idx to give descriptor idx back to hardware; the next index to
    // check is idx+1 (stored in rx_tail).
    desc->status = 0;
    g_e1000.rx_tail = (idx + 1) % E1000_RX_RING_SIZE;
    mmio_write32(mmio, E1000_RDT, idx);

    vxair_log_info("E1000: RX idx=%u len=%u", idx, frame_len);
    return frame_len;
}

int vxair_e1000_loopback_test(void) {
    if (!g_e1000.found || !g_e1000.mmio) return -1;

    volatile uint8_t *mmio = g_e1000.mmio;

    // Enable MAC loopback mode (RCTL LBM = 01b)
    uint32_t rctl_orig = mmio_read32(mmio, E1000_RCTRL);
    uint32_t rctl = rctl_orig;
    rctl &= ~E1000_RCTRL_LBM_MASK;           // Clear old LBM bits
    rctl |= E1000_RCTRL_LBM_MAC;             // Set LBM = 01b (MAC loopback per 82540EM spec)
    mmio_write32(mmio, E1000_RCTRL, rctl);
    vxair_log_info("E1000: LBM enabled RCTL=0x%x", mmio_read32(mmio, E1000_RCTRL));

    // Build a small test frame addressed to our own MAC so any MAC filter accepts it.
    uint8_t test[64];
    memcpy(test, g_e1000.mac_addr, 6);          // dest = own MAC
    memcpy(test + 6, g_e1000.mac_addr, 6);      // src  = own MAC
    test[12] = 0x88; test[13] = 0xB5;             // arbitrary ethertype
    for (int i = 14; i < 46; i++) test[i] = (uint8_t)i;
    // Pad the rest with zeros; send 60 bytes (CRC added by hardware)
    for (int i = 46; i < 60; i++) test[i] = 0;

    vxair_log_info("E1000: LBM sending test frame...");
    int ret = vxair_e1000_send(test, 60);

    // Raw hex dump of descriptor[0] BEFORE polling status, bypassing the struct entirely.
    volatile uint8_t *raw = (volatile uint8_t *)&g_e1000.rx_desc[0];
    vxair_log_info("E1000: raw RX desc[0] after send = "
                   "%02x %02x %02x %02x %02x %02x %02x %02x  "
                   "%02x %02x %02x %02x %02x %02x %02x %02x",
                   raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
                   raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);

    if (ret != 0) {
        vxair_log_info("E1000: LBM send failed");
        mmio_write32(mmio, E1000_RCTRL, rctl_orig);
        return -1;
    }

    // Poll receive for the looped-back frame
    uint8_t rx_buf[2048];
    int received = 0;
    for (int i = 0; i < 1000; i++) {
        if (vxair_e1000_receive(rx_buf, sizeof(rx_buf)) > 0) {
            vxair_log_info("E1000: LBM TEST SUCCESS - looped-back frame received");
            received = 1;
            break;
        }
        for (volatile int d = 0; d < 100; d++) __asm__ volatile("pause");
    }
    if (!received) {
        vxair_log_info("E1000: LBM TEST FAILED - no looped-back frame received");
    }

    // Restore RCTL to normal mode (clear loopback)
    mmio_write32(mmio, E1000_RCTRL, rctl_orig);
    return received ? 0 : -1;
}
