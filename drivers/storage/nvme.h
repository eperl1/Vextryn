#ifndef VXAIR_NVME_H
#define VXAIR_NVME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NVMe Submission Queue Entry
 */
typedef struct {
    uint32_t cdw0;
    uint32_t nsid;
    uint64_t rsvd2;
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} vxair_nvme_sq_entry_t;

/**
 * @brief NVMe Completion Queue Entry
 */
typedef struct {
    uint32_t cdw0;
    uint32_t rsvd1;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
} vxair_nvme_cq_entry_t;

/**
 * @brief Initialize the NVMe storage driver.
 */
void vxair_nvme_init(void);

/**
 * @brief Read sectors from an NVMe drive.
 * @param nsid Namespace ID to read from.
 * @param lba Logical Block Address to start reading.
 * @param count Number of blocks to read.
 * @param buffer Buffer to store read data.
 * @return 0 on success, negative on failure.
 */
int vxair_nvme_read(uint32_t nsid, uint64_t lba, uint32_t count, void* buffer);

/**
 * @brief Write sectors to an NVMe drive.
 * @param nsid Namespace ID to write to.
 * @param lba Logical Block Address to start writing.
 * @param count Number of blocks to write.
 * @param buffer Buffer containing data to write.
 * @return 0 on success, negative on failure.
 */
int vxair_nvme_write(uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NVME_H
