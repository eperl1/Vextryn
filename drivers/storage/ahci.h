#ifndef VXAIR_AHCI_H
#define VXAIR_AHCI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AHCI HBA Port Memory Registers
 */
typedef volatile struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} vxair_ahci_hba_port_t;

/**
 * @brief AHCI HBA Memory Registers
 */
typedef volatile struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t rsv[0xA0 - 0x2C];
    uint8_t vendor[0x100 - 0xA0];
    vxair_ahci_hba_port_t ports[32];
} vxair_ahci_hba_mem_t;

/**
 * @brief Initialize the AHCI storage driver.
 */
void vxair_ahci_init(void);

/**
 * @brief Read sectors from an AHCI SATA drive.
 * @param port Port number to read from.
 * @param lba Logical Block Address.
 * @param count Number of sectors.
 * @param buffer Output buffer.
 * @return 0 on success, non-zero on error.
 */
int vxair_ahci_read(uint8_t port, uint32_t lba, uint32_t count, void *buffer);

/**
 * @brief Write sectors to an AHCI SATA drive.
 * @param port Port number to write to.
 * @param lba Logical Block Address.
 * @param count Number of sectors.
 * @param buffer Input buffer.
 * @return 0 on success, non-zero on error.
 */
int vxair_ahci_write(uint8_t port, uint32_t lba, uint32_t count, const void *buffer);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_AHCI_H
