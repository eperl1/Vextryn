#ifndef ATA_STORAGE_HPP
#define ATA_STORAGE_HPP

#include <stdint.h>
#include <stdbool.h>

// ATA Ports for Primary Bus
#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

static inline uint16_t ata_inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void ata_outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline bool ata_wait_not_busy(uint32_t limit) {
    while (limit > 0) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0xFF || status == 0x00) return false;
        if ((status & 0x80) == 0) return true;
        limit--;
    }
    return false;
}

static inline bool ata_wait_drq_or_error(uint32_t limit) {
    while (limit > 0) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0xFF || status == 0x00) return false;
        if (status & 0x01) return false; // ERR bit
        if (status & 0x20) return false; // DF bit
        if (status & 0x08) return true;  // DRQ bit
        limit--;
    }
    return false;
}

// Simple ATA PIO LBA28 Read
static inline bool ata_read_sector(uint32_t lba, uint8_t* buf) {
    if (!buf) return false;
    if (!ata_wait_not_busy(100000)) return false;
    // Select drive 1 (slave on primary bus) -> 0xF0 means master, 0xE0 means master but bit 4 selects slave.
    // Wait, index=1 in QEMU means it's /dev/hdb (slave on primary bus).
    // The drive selection is 0xE0 for master, 0xF0 for slave (with LBA bit set).
    // Drive bit is bit 4. Master = 0, Slave = 1.
    outb(ATA_DRIVE, 0xF0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, 0x20); // READ SECTORS
    
    if (!ata_wait_not_busy(100000)) return false;
    if (!ata_wait_drq_or_error(100000)) return false;
    
    for (int i = 0; i < 256; i++) {
        uint16_t word = ata_inw(ATA_DATA);
        buf[i * 2] = word & 0xFF;
        buf[i * 2 + 1] = (word >> 8) & 0xFF;
    }
    return true;
}

// Simple ATA PIO LBA28 Write
static inline bool ata_write_sector(uint32_t lba, uint8_t* buf) {
    if (!buf) return false;
    if (!ata_wait_not_busy(100000)) return false;
    // Select drive 1 (slave)
    outb(ATA_DRIVE, 0xF0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, 0x30); // WRITE SECTORS
    
    if (!ata_wait_not_busy(100000)) return false;
    if (!ata_wait_drq_or_error(100000)) return false;
    
    for (int i = 0; i < 256; i++) {
        uint16_t word = buf[i * 2] | (buf[i * 2 + 1] << 8);
        ata_outw(ATA_DATA, word);
    }
    
    // Flush Cache
    outb(ATA_COMMAND, 0xE7);
    if (!ata_wait_not_busy(100000)) return false;
    return true;
}

#endif
