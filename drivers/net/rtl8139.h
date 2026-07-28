#ifndef VXAIR_RTL8139_H
#define VXAIR_RTL8139_H

#include <stdint.h>
#include <stdbool.h>

// ---- Register offsets (I/O port, relative to BAR0 base) ----
#define RTL8139_IDR0      0x00  // MAC address bytes 0-5
#define RTL8139_MAR0      0x08  // Multicast address filter (8 bytes)
#define RTL8139_TSD0      0x10  // TX status descriptor 0 (4 × 32-bit)
#define RTL8139_TSAD0     0x20  // TX start address descriptor 0 (4 × 32-bit)
#define RTL8139_RBSTART   0x30  // RX buffer start address (32-bit physical)
#define RTL8139_CR        0x37  // Command Register (1 byte)
#define RTL8139_CAPR      0x38  // Current Address of Packet Read (16-bit)
#define RTL8139_CBR       0x3A  // Current Buffer Address (16-bit, write-only)
#define RTL8139_IMR       0x3C  // Interrupt Mask (16-bit)
#define RTL8139_ISR       0x3E  // Interrupt Status (16-bit)
#define RTL8139_TCR       0x40  // Transmit Configuration (32-bit)
#define RTL8139_RCR       0x44  // Receive Configuration (32-bit)
#define RTL8139_CONFIG1   0x52  // Configuration 1 (8-bit)

// ---- Command Register (CR, offset 0x37) bits ----
#define RTL8139_CR_BUFE   (1 << 0)  // Buffer Empty (no RX buffer available)
#define RTL8139_CR_RST    (1 << 4)  // Reset
#define RTL8139_CR_RE     (1 << 3)  // Receiver Enable
#define RTL8139_CR_TE     (1 << 2)  // Transmitter Enable

// ---- Receive Configuration (RCR, offset 0x44) bits ----
// Bits 12-11: RX FIFO threshold (00=16B, 01=32B, 10=64B, 11=no threshold)
#define RTL8139_RCR_FTH_NONE   (3 << 11)   // No RX threshold
// Bits 8-10: RX buffer length (000=8K+16, 001=16K+16, 010=32K+16, 011=64K+16)
#define RTL8139_RCR_RBLEN_32K  (2 << 8)    // 32KB+16 (value 010b = 2)
// Bit 7: Wrap (0=don't wrap, 1=wrap to RBSTART when buffer end reached)
#define RTL8139_RCR_WRAP       (1 << 7)
// Bits 5-3: Accept types
#define RTL8139_RCR_AER   (1 << 5)   // Accept Error packets
// Bit 2: Accept Broadcast
#define RTL8139_RCR_AB    (1 << 3)   // Accept Broadcast
// Bit 1: Accept Multicast
#define RTL8139_RCR_AM    (1 << 2)   // Accept Multicast
// Bit 0: Accept Physical Match (unicast to our MAC)
#define RTL8139_RCR_APM   (1 << 1)   // Accept Physical Match
// Accept all = AB | AM | APM
#define RTL8139_RCR_ACCEPT_ALL  (RTL8139_RCR_AB | RTL8139_RCR_AM | RTL8139_RCR_APM)

// ---- Transmit Configuration (TCR, offset 0x40) bits ----
#define RTL8139_TCR_CLRABT  (1 << 0)  // Clear abort on underrun, retransmit
// Max DMA burst: 2048 bytes
#define RTL8139_TCR_MXDMA_2048 (7 << 8)
// Interframe gap: standard 96-bit
#define RTL8139_TCR_IFG_STD    (3 << 24)

// ---- TX Status Descriptor (TSD, offsets 0x10-0x1C) bits ----
#define RTL8139_TSD_OWN   (1 << 13)  // Own bit (0 = driver owns, 1 = NIC owns/transmitting)
#define RTL8139_TSD_TUN   (1 << 14)  // TX FIFO underrun
#define RTL8139_TSD_TOK   (1 << 15)  // TX OK
#define RTL8139_TSD_ERTXTH(n) ((n) << 16)  // Early TX threshold (bytes)
#define RTL8139_TSD_SIZE(n)  ((n) & 0x1FFF)  // TX packet size (bits 0-12)

// ---- Interrupt Status / Mask (ISR/IMR, offsets 0x3E/0x3C) bits ----
#define RTL8139_INT_ROK    (1 << 0)   // Receive OK
#define RTL8139_INT_TOK    (1 << 2)   // Transmit OK
#define RTL8139_INT_RER    (1 << 1)   // Receive Error

// ---- Constants ----
#define RTL8139_NUM_TX_DESC    4
#define RTL8139_RX_BUF_SIZE    (32 * 1024 + 16)  // 32KB + 16 bytes padding
#define RTL8139_MAX_FRAME_SIZE 2048
#define RTL8139_MIN_FRAME_SIZE 60

// ---- Driver state ----
typedef struct {
    bool     found;
    uint16_t io_base;          // I/O port base (BAR0)
    uint8_t  mac_addr[6];

    // RX ring buffer
    uint64_t rx_buf_paddr;     // Physical address
    uint8_t *rx_buf;           // Virtual address (identity-mapped)
    uint16_t rx_offset;        // Our read position within ring

    // TX descriptor slot tracking
    int      tx_next;          // Next free TX descriptor index

    // PCI location
    uint8_t  pci_bus;
    uint8_t  pci_slot;
    uint8_t  pci_func;
} vxair_rtl8139_t;

extern vxair_rtl8139_t g_rtl8139;

// ---- API ----
int      vxair_rtl8139_init(void);
int      vxair_rtl8139_send(const uint8_t *data, uint16_t len);
uint16_t vxair_rtl8139_receive(uint8_t *out_buf, uint16_t max_len);

#endif // VXAIR_RTL8139_H
