#include "rtl8139.h"
#include "../../drivers/bus/bus_pci.h"
#include "../../kernel/hal/hal_pci.h"
#include "../../kernel/core/include/vxair_pmm.h"
#include <string.h>
#include <stddef.h>

extern void vxair_log_info(const char *fmt, ...);

#ifndef RTL8139_VERBOSE
#define RTL8139_VERBOSE 0
#endif

// Global driver state
vxair_rtl8139_t g_rtl8139;

// ---- I/O port helpers ----
static inline uint8_t  inb(uint16_t p)  { uint8_t  v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline uint16_t inw(uint16_t p)  { uint16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline uint32_t inl(uint16_t p)  { uint32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outb(uint16_t p, uint8_t  v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline void outw(uint16_t p, uint16_t v) { __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }
static inline void outl(uint16_t p, uint32_t v) { __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }

// Identity-map helper (kernel identity-maps all physical memory)
static inline void* phys_to_virt(uint64_t paddr) {
    return (void*)(uintptr_t)paddr;
}

static inline uint64_t virt_to_phys(const void *virt) {
    uint64_t v = (uint64_t)(uintptr_t)virt;
    if (v >= 0xFFFFFFFF80000000ULL) return v - 0xFFFFFFFF80000000ULL;
    if (v >= 0x80000000ULL && v < 0x90000000ULL) return v - 0x80000000ULL;
    return v;
}

// Static TX buffers (one per descriptor slot) — avoids stack-buffer DMA issues
// where the buffer could cross page boundaries.
static uint8_t tx_bufs[RTL8139_NUM_TX_DESC][RTL8139_MAX_FRAME_SIZE + 4] __attribute__((aligned(16)));

// Convenience: I/O port offset from io_base
static inline uint8_t  rtl_in8(uint16_t io, uint16_t off)  { return inb(io + off); }
static inline uint16_t rtl_in16(uint16_t io, uint16_t off) { return inw(io + off); }
static inline uint32_t rtl_in32(uint16_t io, uint16_t off) { return inl(io + off); }
static inline void rtl_out8(uint16_t io, uint16_t off, uint8_t  v) { outb(io + off, v); }
static inline void rtl_out16(uint16_t io, uint16_t off, uint16_t v) { outw(io + off, v); }
static inline void rtl_out32(uint16_t io, uint16_t off, uint32_t v) { outl(io + off, v); }

int vxair_rtl8139_init(void) {
    memset(&g_rtl8139, 0, sizeof(g_rtl8139));

    // ---- Find rtl8139 PCI device ----
    uint32_t dev_count = vxair_bus_pci_get_device_count();
    uint8_t bus = 0, slot = 0, func = 0;
    int found = 0;
    for (uint32_t i = 0; i < dev_count; i++) {
        const vxair_pci_device_t *dev = vxair_bus_pci_get_device(i);
        if (dev->vendor_id == 0x10EC && dev->device_id == 0x8139) {
            bus = dev->bus; slot = dev->slot; func = dev->func;
            found = 1;
            vxair_log_info("RTL8139: Found device at bus=%u slot=%u func=%u",
                           bus, slot, func);
            g_rtl8139.pci_bus = bus;
            g_rtl8139.pci_slot = slot;
            g_rtl8139.pci_func = func;
            break;
        }
    }
    if (!found) {
        vxair_log_info("RTL8139: No rtl8139 device found");
        return -1;
    }

    // ---- Read BAR0 (I/O port base) ----
    uint32_t bar0 = vxair_hal_pci_read_config(bus, slot, func, 0x10);
    if (!(bar0 & 1)) {
        vxair_log_info("RTL8139: BAR0 is not I/O (expected I/O BAR)");
        return -1;
    }
    uint16_t io_base = (uint16_t)(bar0 & 0xFFFFFFFCu);
    g_rtl8139.io_base = io_base;
    vxair_log_info("RTL8139: I/O base = 0x%x", io_base);

    // ---- Enable bus mastering ----
    uint32_t cmd = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("RTL8139: PCI CMD before = 0x%x", cmd);
    cmd |= (1 << 2) | (1 << 1);  // Bus Master | Memory Space
    vxair_hal_pci_write_config(bus, slot, func, 0x04, cmd);
    uint32_t cmd_after = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_log_info("RTL8139: PCI CMD after = 0x%x", cmd_after);

    // ---- Power on + software reset ----
    rtl_out8(io_base, RTL8139_CONFIG1, 0x00);
    rtl_out8(io_base, RTL8139_CR, RTL8139_CR_RST);
    // Wait for RST bit to clear (hardware clears it when reset completes)
    for (int i = 0; i < 10000; i++) {
        if (!(rtl_in8(io_base, RTL8139_CR) & RTL8139_CR_RST))
            break;
        for (volatile int d = 0; d < 100; d++) __asm__ volatile("pause");
    }
    vxair_log_info("RTL8139: Reset complete, CR=0x%x", rtl_in8(io_base, RTL8139_CR));

    // ---- Read MAC address ----
    for (int i = 0; i < 6; i++)
        g_rtl8139.mac_addr[i] = rtl_in8(io_base, RTL8139_IDR0 + i);
    vxair_log_info("RTL8139: MAC=%02x:%02x:%02x:%02x:%02x:%02x",
                   g_rtl8139.mac_addr[0], g_rtl8139.mac_addr[1],
                   g_rtl8139.mac_addr[2], g_rtl8139.mac_addr[3],
                   g_rtl8139.mac_addr[4], g_rtl8139.mac_addr[5]);

    // ---- Allocate RX ring buffer via PMM ----
    size_t rx_pages = (RTL8139_RX_BUF_SIZE + 4095) / 4096;
    g_rtl8139.rx_buf_paddr = vxair_pmm_alloc_pages(rx_pages);
    g_rtl8139.rx_buf = (uint8_t *)phys_to_virt(g_rtl8139.rx_buf_paddr);
    memset(g_rtl8139.rx_buf, 0, RTL8139_RX_BUF_SIZE);
    vxair_log_info("RTL8139: RX buffer phys=0x%llx size=%u",
                   (unsigned long long)g_rtl8139.rx_buf_paddr,
                   RTL8139_RX_BUF_SIZE);

    // ---- Configure RX ----
    // Set RBSTART (physical address of RX ring buffer)
    rtl_out32(io_base, RTL8139_RBSTART, (uint32_t)g_rtl8139.rx_buf_paddr);
    // Initialize CAPR to 0 — the hardware may have a stale/non-zero value after
    // reset, which would make the polling loop think data is available at a bogus
    // offset. Explicitly tell the hardware we've consumed nothing yet.
    rtl_out16(io_base, RTL8139_CAPR, 0);
    // Initialize CBR to 0 — sets the buffer boundary/starting point.
    rtl_out16(io_base, RTL8139_CBR, 0);
    vxair_log_info("RTL8139: RBSTART = 0x%x (readback=0x%x), CAPR init=0x%x",
                   (uint32_t)g_rtl8139.rx_buf_paddr,
                   rtl_in32(io_base, RTL8139_RBSTART),
                   rtl_in16(io_base, RTL8139_CAPR));

    // Set RX config: accept all packets, 32KB+16 buffer, wrap
    uint32_t rcr = RTL8139_RCR_ACCEPT_ALL
                 | RTL8139_RCR_WRAP
                 | RTL8139_RCR_FTH_NONE
                 | RTL8139_RCR_RBLEN_32K;
    rtl_out32(io_base, RTL8139_RCR, rcr);
    vxair_log_info("RTL8139: RCR = 0x%x (readback=0x%x)",
                   rcr, rtl_in32(io_base, RTL8139_RCR));

    // ---- Configure TX ----
    uint32_t tcr = RTL8139_TCR_CLRABT | RTL8139_TCR_MXDMA_2048 | RTL8139_TCR_IFG_STD;
    rtl_out32(io_base, RTL8139_TCR, tcr);
    vxair_log_info("RTL8139: TCR = 0x%x (readback=0x%x)",
                   tcr, rtl_in32(io_base, RTL8139_TCR));

    // ---- Mask all interrupts (polling mode) ----
    rtl_out16(io_base, RTL8139_IMR, 0x0000);

    // ---- Enable RX and TX ----
    uint8_t cr = RTL8139_CR_RE | RTL8139_CR_TE;
    rtl_out8(io_base, RTL8139_CR, cr);
    vxair_log_info("RTL8139: CR set to 0x%x (readback=0x%x) [RE=%d TE=%d]",
                   cr, rtl_in8(io_base, RTL8139_CR),
                   (rtl_in8(io_base, RTL8139_CR) >> 3) & 1,
                   (rtl_in8(io_base, RTL8139_CR) >> 2) & 1);

    // Initialize RX read pointer
    g_rtl8139.rx_offset = 0;
    g_rtl8139.tx_next = 0;
    g_rtl8139.found = true;

    vxair_log_info("RTL8139: Init OK");
    return 0;
}

int vxair_rtl8139_send(const uint8_t *data, uint16_t len) {
    if (!g_rtl8139.found) return -1;
    if (len > RTL8139_MAX_FRAME_SIZE || len < RTL8139_MIN_FRAME_SIZE) return -1;

    uint16_t io = g_rtl8139.io_base;

    int idx = g_rtl8139.tx_next;
    g_rtl8139.tx_next = (idx + 1) % RTL8139_NUM_TX_DESC;

    // Use a static per-descriptor buffer (avoids stack page-boundary DMA issues)
    memcpy(tx_bufs[idx], data, len);
    uint32_t tx_phys = (uint32_t)virt_to_phys(tx_bufs[idx]);
    rtl_out32(io, RTL8139_TSAD0 + idx * 4, tx_phys);

    // Write size + early TX threshold to TSD
    // TOK, TUN, and OWN are set by hardware — don't set them here
    // ERTXTH: start TX after 64 bytes in FIFO (0 << 16 = no early threshold = DMA whole packet)
    uint32_t tsd = RTL8139_TSD_SIZE(len);
    rtl_out32(io, RTL8139_TSD0 + idx * 4, tsd);

    if (RTL8139_VERBOSE) {
        vxair_log_info("RTL8139: TX posted idx=%u len=%u phys=0x%x",
                       idx, len, tx_phys);
    }

    // Poll for completion — hardware clears OWN and sets TOK when done
    for (int p = 0; p < 5000; p++) {
        uint32_t status = rtl_in32(io, RTL8139_TSD0 + idx * 4);
        if (status & RTL8139_TSD_TOK) {
            if (RTL8139_VERBOSE) {
                vxair_log_info("RTL8139: TX done idx=%u iterations=%d", idx, p);
            }
            return 0;
        }
        if (!(status & RTL8139_TSD_OWN)) {
            vxair_log_info("RTL8139: TX OWN cleared (not OK) idx=%u status=0x%x", idx, status);
            return -1;
        }
        for (volatile int d = 0; d < 100; d++) __asm__ volatile("pause");
    }

    uint32_t final_status = rtl_in32(io, RTL8139_TSD0 + idx * 4);
    vxair_log_info("RTL8139: TX timeout idx=%u status=0x%x", idx, final_status);
    return -1;
}

uint16_t vxair_rtl8139_receive(uint8_t *out_buf, uint16_t max_len) {
    if (!g_rtl8139.found || !out_buf) return 0;

    uint16_t io = g_rtl8139.io_base;

    uint8_t cr = rtl_in8(io, RTL8139_CR);
    if (cr & RTL8139_CR_BUFE) {
        if (RTL8139_VERBOSE) {
            static int rx_empty_count = 0;
            if (++rx_empty_count <= 3 || rx_empty_count % 128 == 0) {
                vxair_log_info("RTL8139: RX empty #%d CR=0x%x CAPR=0x%x CBR=0x%x our_off=%u",
                               rx_empty_count, cr, rtl_in16(io, RTL8139_CAPR),
                               rtl_in16(io, RTL8139_CBR), g_rtl8139.rx_offset);
            }
        }
        return 0;
    }

    uint16_t rx_offset = g_rtl8139.rx_offset;
    uint16_t offset_norm = rx_offset % RTL8139_RX_BUF_SIZE;

    // Read the 4-byte header at our current position
    // Header format: status[2] (little-endian) | length[2] (little-endian)
    uint16_t *hdr = (uint16_t *)(g_rtl8139.rx_buf + offset_norm);
    uint16_t status = hdr[0];
    uint16_t pkt_len = hdr[1];

    // Check for valid packet (ROK bit set)
    if (!(status & RTL8139_INT_ROK)) {
        vxair_log_info("RTL8139: RX bad status=0x%x at offset=%u (CR=0x%x CBR=0x%x)",
                       status, offset_norm, cr, rtl_in16(io, RTL8139_CBR));
        // CAPR may report a spurious non-zero value before any data arrives
        // (hardware offset quirk). Don't advance rx_offset — instead, tell the
        // hardware we've consumed up to our current position.
        rtl_out16(io, RTL8139_CAPR, offset_norm);
        return 0;
    }

    if (pkt_len < 64 || pkt_len > RTL8139_MAX_FRAME_SIZE) {
        vxair_log_info("RTL8139: RX bad length=%u at offset=%u", pkt_len, offset_norm);
        g_rtl8139.rx_offset = (rx_offset + 4) % RTL8139_RX_BUF_SIZE;
        rtl_out16(io, RTL8139_CAPR, (g_rtl8139.rx_offset - 16) & 0xFFFF);
        return 0;
    }

    // Copy packet data (after the 4-byte header)
    uint16_t copy_len = (pkt_len > max_len) ? max_len : pkt_len;
    uint16_t data_offset = (offset_norm + 4) % RTL8139_RX_BUF_SIZE;

    // Handle wrap: data may cross buffer end
    if (data_offset + copy_len <= RTL8139_RX_BUF_SIZE) {
        memcpy(out_buf, g_rtl8139.rx_buf + data_offset, copy_len);
    } else {
        uint16_t first_chunk = RTL8139_RX_BUF_SIZE - data_offset;
        memcpy(out_buf, g_rtl8139.rx_buf + data_offset, first_chunk);
        memcpy(out_buf + first_chunk, g_rtl8139.rx_buf, copy_len - first_chunk);
    }

    // Advance our read pointer past this packet
    // Each packet consumes: 4-byte header + packet length, rounded up to 4-byte boundary
    uint16_t consumed = (pkt_len + 4 + 3) & ~3;
    uint16_t new_offset = rx_offset + consumed;

    // Update the CAPR register to tell hardware we've consumed up to this point
    // CAPR must be written with (our position - 16) to leave 16 bytes of padding
    uint16_t new_capr = (new_offset - 16) & 0xFFFF;
    rtl_out16(io, RTL8139_CAPR, new_capr);

    g_rtl8139.rx_offset = new_offset;

    if (RTL8139_VERBOSE) {
        vxair_log_info("RTL8139: RX pkt_len=%u status=0x%x off=%u->%u cbr=%u capr_new=%u",
                       pkt_len, status, rx_offset, new_offset, rtl_in16(io, RTL8139_CBR), new_capr);
    }

    return copy_len;
}
