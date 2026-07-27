#include "virtio_net.h"
#include "../../drivers/bus/bus_pci.h"
#include "../../kernel/hal/hal_pci.h"
#include "../../kernel/core/include/vxair_pmm.h"
#include <string.h>

extern void vxair_log_info(const char *fmt, ...);

// Port I/O helpers
static inline uint8_t  inb(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline uint16_t inw(uint16_t p) { uint16_t v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline uint32_t inl(uint16_t p) { uint32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline void outw(uint16_t p, uint16_t v) { __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }
static inline void outl(uint16_t p, uint32_t v) { __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }

// Identity-mapped physical-to-virtual (kernel boots with P4[0] and P4[511] both identity-mapped)
static inline void* phys_to_virt(uint64_t paddr) { return (void*)(uintptr_t)paddr; }

// Virtual-to-physical (for higher-half addresses only — .bss lives there)
static inline uint64_t virt_to_phys(const void *virt) {
    uint64_t v = (uint64_t)(uintptr_t)virt;
    if (v >= 0xFFFFFFFF80000000ULL) return v - 0xFFFFFFFF80000000ULL;
    return v; // identity-mapped
}

// State
vxair_virtio_net_t g_virtio_net;

// PMM-allocated virtqueues (NOT in .bss — allocated from kernel's physical allocator)
static vxair_virtq_t *tx_vq = NULL;
static vxair_virtq_t *rx_vq = NULL;
static vxair_paddr_t tx_vq_paddr = 0;
static vxair_paddr_t rx_vq_paddr = 0;

// PMM-allocated TX frame buffer (not in .bss)
static uint8_t *tx_frame = NULL;
static vxair_paddr_t tx_frame_paddr = 0;

// RX buffers — allocated as a single PMM block
static uint8_t (*rx_bufs)[VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE] = NULL;
static vxair_paddr_t rx_bufs_paddr = 0;

#define REG_DEVICE_FEATURES  0x00
#define REG_GUEST_FEATURES   0x04
#define REG_QUEUE_PFN        0x08
#define REG_QUEUE_SIZE       0x0C
#define REG_QUEUE_SEL        0x0E
#define REG_QUEUE_NOTIFY     0x10
#define REG_DEVICE_STATUS    0x12
#define REG_ISR              0x13
#define REG_DEVICE_CONFIG    0x14

int vxair_virtio_net_init(void) {
    memset(&g_virtio_net, 0, sizeof(g_virtio_net));

    // ---- Allocate PMM pages for all buffers (not .bss) ----
    size_t vring_bytes = (sizeof(vxair_virtq_t) + 4095) & ~4095; // Round up to page
    size_t vring_pages = vring_bytes / 4096;

    tx_vq_paddr = vxair_pmm_alloc_pages(vring_pages);
    rx_vq_paddr = vxair_pmm_alloc_pages(vring_pages);
    tx_frame_paddr = vxair_pmm_alloc_page();

    // RX buffers: 64 buffers × (12 header + 2048 frame) = 131840 bytes = 33 pages
    size_t rx_bufs_pages = (VIRTIO_NET_QUEUE_SIZE * (VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE) + 4095) / 4096;
    rx_bufs_paddr = vxair_pmm_alloc_pages(rx_bufs_pages);

    // Convert physical to virtual via identity mapping
    tx_vq = (vxair_virtq_t*)phys_to_virt(tx_vq_paddr);
    rx_vq = (vxair_virtq_t*)phys_to_virt(rx_vq_paddr);
    tx_frame = (uint8_t*)phys_to_virt(tx_frame_paddr);
    rx_bufs = (uint8_t(*)[VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE])phys_to_virt(rx_bufs_paddr);

    // Zero them out
    memset(tx_vq, 0, vring_bytes);
    memset(rx_vq, 0, vring_bytes);
    memset(tx_frame, 0, VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE);
    memset(rx_bufs, 0, VIRTIO_NET_QUEUE_SIZE * (VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE));

    // ---- ADDRESS COMPARISON DIAGNOSTIC ----
    // PMM returns physical address. virt_to_phys() on the identity-mapped virtual
    // pointer should give the SAME physical address. Any mismatch = root cause.
    vxair_log_info("NET: ADDR COMPARE:");
    vxair_log_info("NET:   tx_vq:  PMM=0x%x  V2P(0x%p)=0x%x",
                   (uint32_t)tx_vq_paddr, (void*)tx_vq, (uint32_t)virt_to_phys(tx_vq));
    vxair_log_info("NET:   rx_vq:  PMM=0x%x  V2P(0x%p)=0x%x",
                   (uint32_t)rx_vq_paddr, (void*)rx_vq, (uint32_t)virt_to_phys(rx_vq));
    vxair_log_info("NET:   tx_frm: PMM=0x%x  V2P(0x%p)=0x%x",
                   (uint32_t)tx_frame_paddr, (void*)tx_frame, (uint32_t)virt_to_phys(tx_frame));
    vxair_log_info("NET:   rx_buf: PMM=0x%x  V2P(0x%p)=0x%x",
                   (uint32_t)rx_bufs_paddr, (void*)rx_bufs, (uint32_t)virt_to_phys(rx_bufs));

    // Verify: virt_to_phys on PMM-allocated pages should MATCH PMM address
    if (tx_vq_paddr != virt_to_phys(tx_vq))
        vxair_log_info("NET: *** MISMATCH tx_vq! PMM=0x%x != V2P=0x%x ***",
                       (uint32_t)tx_vq_paddr, (uint32_t)virt_to_phys(tx_vq));
    if (rx_vq_paddr != virt_to_phys(rx_vq))
        vxair_log_info("NET: *** MISMATCH rx_vq! PMM=0x%x != V2P=0x%x ***",
                       (uint32_t)rx_vq_paddr, (uint32_t)virt_to_phys(rx_vq));
    if (tx_frame_paddr != virt_to_phys(tx_frame))
        vxair_log_info("NET: *** MISMATCH tx_frame! PMM=0x%x != V2P=0x%x ***",
                       (uint32_t)tx_frame_paddr, (uint32_t)virt_to_phys(tx_frame));
    if (rx_bufs_paddr != virt_to_phys(rx_bufs))
        vxair_log_info("NET: *** MISMATCH rx_bufs! PMM=0x%x != V2P=0x%x ***",
                       (uint32_t)rx_bufs_paddr, (uint32_t)virt_to_phys(rx_bufs));

    // Find virtio device
    uint32_t dev_count = vxair_bus_pci_get_device_count();
    uint8_t bus = 0, slot = 0, func = 0;
    int found = 0;
    for (uint32_t i = 0; i < dev_count; i++) {
        const vxair_pci_device_t *dev = vxair_bus_pci_get_device(i);
        if (dev->vendor_id == 0x1AF4) {
            bus = dev->bus; slot = dev->slot; func = dev->func;
            found = 1;
            break;
        }
    }
    if (!found) { vxair_log_info("NET: No virtio device"); return -1; }
    vxair_log_info("NET: Found virtio at bus=%u slot=%u", bus, slot);

    // BAR0
    uint32_t bar0 = vxair_hal_pci_read_config(bus, slot, func, 0x10);
    if (!(bar0 & 1)) { vxair_log_info("NET: BAR0 not I/O"); return -1; }
    g_virtio_net.io_base = bar0 & 0xFFFC;
    vxair_log_info("NET: I/O base = 0x%x", g_virtio_net.io_base);

    // Enable bus master + I/O + memory
    uint32_t cmd = vxair_hal_pci_read_config(bus, slot, func, 0x04);
    vxair_hal_pci_write_config(bus, slot, func, 0x04, cmd | 7);

    uint16_t io = g_virtio_net.io_base;

    // Reset + ACK + DRIVER
    outb(io + REG_DEVICE_STATUS, 0);
    inb(io + REG_ISR);
    outb(io + REG_DEVICE_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    // Feature negotiation (only MAC + STATUS)
    uint32_t dev_features = inl(io + REG_DEVICE_FEATURES);
    uint32_t guest_features = 0;
    if (dev_features & (1 << 5))  guest_features |= (1 << 5);
    if (dev_features & (1 << 16)) guest_features |= (1 << 16);
    outl(io + REG_GUEST_FEATURES, guest_features);
    vxair_log_info("NET: Features=0x%x Negotiated=0x%x", dev_features, guest_features);

    outb(io + REG_DEVICE_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
    if (!(inb(io + REG_DEVICE_STATUS) & VIRTIO_STATUS_FEATURES_OK)) { vxair_log_info("NET: FEATURES_OK rejected"); return -1; }

    // MAC
    for (int i = 0; i < 6; i++) g_virtio_net.mac_addr[i] = inb(io + REG_DEVICE_CONFIG + i);
    vxair_log_info("NET: MAC=%x:%x:%x:%x:%x:%x",
                   g_virtio_net.mac_addr[0], g_virtio_net.mac_addr[1],
                   g_virtio_net.mac_addr[2], g_virtio_net.mac_addr[3],
                   g_virtio_net.mac_addr[4], g_virtio_net.mac_addr[5]);

    // --- Queue setup (q0=RX, q1=TX) ---
    // RX queue (q0)
    outw(io + REG_QUEUE_SEL, 0);
    uint16_t rx_qsz = inw(io + REG_QUEUE_SIZE);
    if (rx_qsz == 0 || rx_qsz > VIRTIO_NET_QUEUE_SIZE) rx_qsz = VIRTIO_NET_QUEUE_SIZE;
    g_virtio_net.rx_vq = rx_vq;

    // Populate RX descriptors (buffers are PMM-allocated, not .bss)
    for (int i = 0; i < rx_qsz; i++) {
        rx_vq->desc[i].addr  = virt_to_phys(rx_bufs[i]);
        rx_vq->desc[i].len   = VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE;
        rx_vq->desc[i].flags = VIRTQ_DESC_F_WRITE;
        rx_vq->desc[i].next  = 0;
        rx_vq->avail.ring[i] = i;
    }
    rx_vq->avail.idx = rx_qsz;
    g_virtio_net.rx_next_used = 0;
    g_virtio_net.rx_avail_repost = rx_qsz;

    __asm__ volatile("mfence" ::: "memory");
    outl(io + REG_QUEUE_PFN, (uint32_t)(rx_vq_paddr >> 12));

    // TX queue (q1)
    outw(io + REG_QUEUE_SEL, 1);
    uint16_t tx_qsz = inw(io + REG_QUEUE_SIZE);
    if (tx_qsz == 0 || tx_qsz > VIRTIO_NET_QUEUE_SIZE) tx_qsz = VIRTIO_NET_QUEUE_SIZE;
    g_virtio_net.tx_vq = tx_vq;
    outl(io + REG_QUEUE_PFN, (uint32_t)(tx_vq_paddr >> 12));

    // DRIVER_OK
    outb(io + REG_DEVICE_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                                 VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);
    vxair_log_info("NET: Status=0x%x", inb(io + REG_DEVICE_STATUS));

    g_virtio_net.found = true;
    vxair_log_info("NET: Init OK (RX q0=%u TX q1=%u)", rx_qsz, tx_qsz);
    return 0;
}

int vxair_virtio_net_send(const uint8_t *data, uint16_t len) {
    if (!g_virtio_net.found || !g_virtio_net.tx_vq) return -1;
    if (len > VIRTIO_NET_FRAME_SIZE) return -1;

    // Build tx_frame: [header (12 bytes zero)] + [data]
    memset(tx_frame, 0, VIRTIO_NET_HDR_SIZE);
    memcpy(tx_frame + VIRTIO_NET_HDR_SIZE, data, len);

    vxair_virtq_t *vq = g_virtio_net.tx_vq;
    uint16_t total_len = VIRTIO_NET_HDR_SIZE + len;

    vq->desc[0].addr  = tx_frame_paddr;  // Use PMM physical address directly!
    vq->desc[0].len   = total_len;
    vq->desc[0].flags = 0;

    uint16_t idx = vq->avail.idx;
    vq->avail.ring[idx & (VIRTIO_NET_QUEUE_SIZE - 1)] = 0;
    __asm__ volatile("mfence" ::: "memory");
    vq->avail.idx = idx + 1;

    // N1-FIX-4: Explicitly re-select TX queue (q1) and confirm value before notify
    outw(g_virtio_net.io_base + REG_QUEUE_SEL, 1);
    uint16_t qsel_check = inw(g_virtio_net.io_base + REG_QUEUE_SEL);

    // Queue Select must be 1 before we notify queue 1 — log confirmation
    vxair_log_info("NET: TX qsel=%u before notify (expect 1)", qsel_check);

    // Notify TX queue (q1)
    outw(g_virtio_net.io_base + REG_QUEUE_NOTIFY, 1);

    // Read ISR AFTER notify to check if device processed the queue
    uint8_t isr_post = inb(g_virtio_net.io_base + REG_ISR);

    vxair_log_info("NET: TX sent avail=%u len=%u total=%u desc=0x%x qsel=%u isr=0x%x",
                   vq->avail.idx, len, total_len, (uint32_t)vq->desc[0].addr, qsel_check, isr_post);

    // Poll for TX completion
    uint16_t start = vq->used.idx;
    __asm__ volatile("mfence" ::: "memory");
    for (int p = 0; p < 5000; p++) {
        __asm__ volatile(""); // compiler barrier
        if (vq->used.idx != start) {
            vxair_log_info("NET: TX done! used=%u isr=0x%x", vq->used.idx, inb(g_virtio_net.io_base + REG_ISR));
            break;
        }
        for (volatile int d = 0; d < 100; d++) __asm__ volatile("pause");
    }
    if (vq->used.idx == start)
        vxair_log_info("NET: TX NOT done (used=%u) isr_post=0x%x isr_now=0x%x",
                       start, isr_post, inb(g_virtio_net.io_base + REG_ISR));
    return 0;
}

uint16_t vxair_virtio_net_receive(uint8_t *out_buf, uint16_t max_len) {
    if (!g_virtio_net.found || !g_virtio_net.rx_vq || !out_buf) return 0;

    vxair_virtq_t *vq = g_virtio_net.rx_vq;
    __asm__ volatile("mfence" ::: "memory");
    uint16_t used_entries = vq->used.idx;
    if (g_virtio_net.rx_next_used == used_entries) return 0;

    uint16_t u_idx = g_virtio_net.rx_next_used & (VIRTIO_NET_QUEUE_SIZE - 1);
    uint32_t desc_id = vq->used.ring[u_idx].id;
    uint32_t desc_len = vq->used.ring[u_idx].len;

    uint16_t frame_len = 0;
    if (desc_len > VIRTIO_NET_HDR_SIZE && desc_id < VIRTIO_NET_QUEUE_SIZE) {
        frame_len = desc_len - VIRTIO_NET_HDR_SIZE;
        if (frame_len > max_len) frame_len = max_len;
        memcpy(out_buf, rx_bufs[desc_id] + VIRTIO_NET_HDR_SIZE, frame_len);
    }

    if (desc_id < VIRTIO_NET_QUEUE_SIZE) {
        uint16_t av = g_virtio_net.rx_avail_repost;
        vq->avail.ring[av & (VIRTIO_NET_QUEUE_SIZE - 1)] = desc_id;
        __asm__ volatile("mfence" ::: "memory");
        vq->avail.idx = av + 1;
        g_virtio_net.rx_avail_repost = av + 1;
        outl(g_virtio_net.io_base + REG_QUEUE_NOTIFY, 0); // Try 32-bit notify
        outw(g_virtio_net.io_base + REG_QUEUE_NOTIFY, 0); // And 16-bit fallback
    }

    g_virtio_net.rx_next_used++;
    vxair_log_info("NET: RX d=%u len=%u frame=%u", desc_id, desc_len, frame_len);
    return frame_len;
}
