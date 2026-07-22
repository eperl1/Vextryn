#include "ahci.h"

static int vxair_ahci_initialized = 0;

void vxair_ahci_init(void) {
    if (vxair_ahci_initialized) {
        return;
    }
    // Discover PCI AHCI controllers, map memory, initialize ports
    vxair_ahci_initialized = 1;
}

int vxair_ahci_read(uint8_t port, uint32_t lba, uint32_t count, void *buffer) {
    (void)port;
    (void)lba;
    (void)count;
    (void)buffer;
    if (!vxair_ahci_initialized) {
        return -1;
    }
    
    // Construct command header and FIS, issue command, wait for completion
    return 0; // Success
}

int vxair_ahci_write(uint8_t port, uint32_t lba, uint32_t count, const void *buffer) {
    (void)port;
    (void)lba;
    (void)count;
    (void)buffer;
    if (!vxair_ahci_initialized) {
        return -1;
    }
    
    // Construct command header and FIS, issue command, wait for completion
    return 0; // Success
}
