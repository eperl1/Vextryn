#include "nvme.h"

static int vxair_nvme_initialized = 0;

void vxair_nvme_init(void) {
    if (vxair_nvme_initialized) {
        return;
    }
    // Discover PCI NVMe controllers, configure admin queues
    vxair_nvme_initialized = 1;
}

int vxair_nvme_read(uint32_t nsid, uint64_t lba, uint32_t count, void* buffer) {
    (void)nsid;
    (void)lba;
    (void)count;
    (void)buffer;
    if (!vxair_nvme_initialized) {
        return -1;
    }
    
    // Construct SQ entry for read command, ring doorbell, wait for CQ
    return 0; // Success
}

int vxair_nvme_write(uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer) {
    (void)nsid;
    (void)lba;
    (void)count;
    (void)buffer;
    if (!vxair_nvme_initialized) {
        return -1;
    }
    
    // Construct SQ entry for write command, ring doorbell, wait for CQ
    return 0; // Success
}
