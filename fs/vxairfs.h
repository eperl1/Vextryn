#ifndef VXAIR_VXAIRFS_H
#define VXAIR_VXAIRFS_H

#include <stdint.h>
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VxAirFS Superblock structure.
 */
typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t num_blocks;
    uint32_t num_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t root_inode;
} vxair_vxairfs_superblock_t;

/**
 * @brief Initialize the VxAirFS filesystem driver.
 */
void vxair_vxairfs_init(void);

/**
 * @brief Mount a VxAirFS filesystem.
 * @param device_node The device block node to mount.
 * @return The root VFS node of the mounted filesystem.
 */
vxair_vfs_node_t* vxair_vxairfs_mount(vxair_vfs_node_t* device_node);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_VXAIRFS_H
