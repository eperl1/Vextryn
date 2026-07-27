#ifndef VXAIR_E1000_H
#define VXAIR_E1000_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===== Register Offsets (82540EM / e1000) =====
// All offsets relative to MMIO base (PCI BAR0)
#define E1000_CTRL    0x0000  // Device Control
#define E1000_STATUS  0x0008  // Device Status
#define E1000_EERD    0x0014  // EEPROM Read
#define E1000_IMS     0x00D0  // Interrupt Mask Set/Read
#define E1000_RCTRL   0x0100  // Receive Control
#define E1000_TCTRL   0x0400  // Transmit Control
#define E1000_TIPG    0x0410  // Transmit Inter-Packet Gap
#define E1000_MTA     0x5200  // Multicast Table Array (128 bytes)
#define E1000_RDBAL   0x2800  // RX Descriptor Base Low
#define E1000_RDBAH   0x2804  // RX Descriptor Base High
#define E1000_RDLEN   0x2808  // RX Descriptor Length
#define E1000_RDH     0x2810  // RX Descriptor Head
#define E1000_RDT     0x2818  // RX Descriptor Tail
#define E1000_RXDCTL  0x2828  // RX Descriptor Control
#define E1000_TDBAL   0x3800  // TX Descriptor Base Low
#define E1000_TDBAH   0x3804  // TX Descriptor Base High
#define E1000_TDLEN   0x3808  // TX Descriptor Length
#define E1000_TDH     0x3810  // TX Descriptor Head
#define E1000_TDT     0x3818  // TX Descriptor Tail
#define E1000_RAL     0x5400  // Receive Address Low
#define E1000_RAH     0x5404  // Receive Address High
#define E1000_RAH_AV  (1U << 31)  // Address Valid bit in RAH

// ===== CTRL Register Bits =====
#define E1000_CTRL_RST    (1 << 26)  // Device Reset
#define E1000_CTRL_SLU    (1 << 6)   // Set Link Up
#define E1000_CTRL_FD     (1 << 0)   // Full Duplex

// ===== RCTRL Register Bits =====
#define E1000_RCTRL_EN    (1 << 1)   // Receiver Enable
#define E1000_RCTRL_SBP   (1 << 2)   // Store Bad Packets
#define E1000_RCTRL_UPE   (1 << 3)   // Unicast Promiscuous Enable
#define E1000_RCTRL_MPE   (1 << 4)   // Multicast Promiscuous Enable
#define E1000_RCTRL_LPE   (1 << 5)   // Long Packet Enable
#define E1000_RCTRL_LBM_MASK   (3 << 6)  // Loopback Mode field mask
#define E1000_RCTRL_LBM_MAC    (1 << 6)  // MAC loopback (01b) per 82540EM spec
#define E1000_RCTRL_BAM   (1 << 15)  // Broadcast Accept Mode
#define E1000_RCTRL_SECRC (1 << 26)  // Strip Ethernet CRC from incoming packets

// BSIZE: bits 16-17  00=2048, 01=1024, 10=512, 11=256
#define E1000_RCTRL_BSIZE_2048 (0 << 16)
#define E1000_RCTRL_BSIZE_1024 (1 << 16)
#define E1000_RCTRL_BSIZE_512  (2 << 16)
#define E1000_RCTRL_BSIZE_256  (3 << 16)

// ===== RXDCTL Register Bits =====
// Default threshold configuration used by Linux/standard drivers.
// PTHRESH=1, HTHRESH=1, WTHRESH=1, GRAN=1 (descriptor granularity).
#define E1000_RXDCTL_DEFAULT 0x01010101U

// ===== TCTRL Register Bits =====
#define E1000_TCTRL_EN    (1 << 1)   // Transmit Enable
#define E1000_TCTRL_PSP   (1 << 3)   // Pad Short Packets
#define E1000_TCTRL_CT    (0x10 << 4)   // Collision Threshold (standard 0x10)
#define E1000_TCTRL_COLD  (0x40 << 12)  // Collision Distance full-duplex (standard 0x40)

// ===== TIPG standard value (IPGT=10, IPGR1=8, IPGR2=6) =====
#define E1000_TIPG_DEFAULT 0x0060200AU

// ===== TX Descriptor Command Bits =====
#define E1000_TXD_CMD_EOP  (1 << 0)  // End of Packet
#define E1000_TXD_CMD_IFCS (1 << 1)  // Insert FCS/CRC
#define E1000_TXD_CMD_RS   (1 << 3)  // Report Status
#define E1000_TXD_CMD_IDE  (1 << 7)  // Interrupt Delay Enable

// ===== TX Descriptor Status Bits =====
#define E1000_TXD_STAT_DD  (1 << 0)  // Descriptor Done

// ===== RX Descriptor Status Bits =====
#define E1000_RXD_STAT_DD  (1 << 0)  // Descriptor Done
#define E1000_RXD_STAT_EOP (1 << 1)  // End of Packet

// ===== EERD Bits =====
#define E1000_EERD_START   (1 << 0)  // Start Read
#define E1000_EERD_DONE    (1 << 1)  // Read Done
#define E1000_EERD_ADDR_SHIFT 8      // Address at bits 8-10 per Intel 82540EM spec

// ===== Ring Sizes =====
#define E1000_TX_RING_SIZE   16
#define E1000_RX_RING_SIZE   16
#define E1000_BUFFER_SIZE    2048

// ===== Descriptor Structs =====

// Transmit descriptor (16 bytes, must be 16-byte aligned)
typedef struct __attribute__((packed, aligned(16))) {
    uint64_t addr;         // Physical address of packet buffer
    uint16_t length;       // Packet length
    uint8_t  cso;          // Checksum Offset
    uint8_t  cmd;          // Command bits
    uint8_t  status;       // Status bits (written back by hardware)
    uint8_t  css;          // Checksum Start
    uint16_t special;      // Special (VLAN, etc.)
} e1000_tx_desc_t;

// Receive descriptor (16 bytes, must be 16-byte aligned)
typedef struct __attribute__((packed, aligned(16))) {
    uint64_t addr;         // Physical address of packet buffer
    uint16_t length;       // Packet length (written by hardware)
    uint16_t checksum;     // Packet checksum (written by hardware)
    uint8_t  status;       // Status bits (written by hardware)
    uint8_t  errors;       // Error bits (written by hardware)
    uint16_t special;      // Special (VLAN)
} e1000_rx_desc_t;

// ===== Device State =====
typedef struct __attribute__((aligned(16))) {
    bool found;
    uint64_t mmio_paddr;        // Physical address of MMIO region
    volatile uint8_t *mmio;     // Virtual (identity-mapped) pointer to MMIO

    uint8_t mac_addr[6];
    uint8_t pci_bus;
    uint8_t pci_slot;
    uint8_t pci_func;

    // TX ring
    e1000_tx_desc_t *tx_desc;   // Virtual pointer to TX descriptor ring
    uint64_t tx_desc_paddr;     // Physical address of TX descriptor ring
    uint16_t tx_tail;           // Next descriptor index (TDT)

    // RX ring
    e1000_rx_desc_t *rx_desc;   // Virtual pointer to RX descriptor ring
    uint64_t rx_desc_paddr;     // Physical address of RX descriptor ring
    uint16_t rx_tail;           // Descriptor index to check next

    // Packet buffers
    uint8_t *tx_bufs;           // TX packet buffers (ring_size × buffer_size)
    uint64_t tx_bufs_paddr;
    uint8_t *rx_bufs;           // RX packet buffers
    uint64_t rx_bufs_paddr;
} vxair_e1000_t;

extern vxair_e1000_t g_e1000;

// ===== Driver Interface =====
int      vxair_e1000_init(void);
int      vxair_e1000_send(const uint8_t *data, uint16_t len);
uint16_t vxair_e1000_receive(uint8_t *out_buf, uint16_t max_len);
int      vxair_e1000_loopback_test(void);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_E1000_H
