#ifndef VXAIR_RAMDISK_H
#define VXAIR_RAMDISK_H

#include <stdint.h>
#include <stddef.h>
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the RAM disk driver.
 * @param start_address The starting memory address of the RAM disk.
 * @param size The size of the RAM disk in bytes.
 */
void vxair_ramdisk_init(void* start_address, size_t size);

/**
 * @brief Get the VFS node for the RAM disk.
 * @return The VFS node representing the block device.
 */
vxair_vfs_node_t* vxair_ramdisk_get_node(void);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_RAMDISK_H
