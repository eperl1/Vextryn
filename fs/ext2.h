#ifndef VXAIR_EXT2_H
#define VXAIR_EXT2_H

#include <stdint.h>
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EXT2 Superblock structure.
 */
typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
} vxair_ext2_superblock_t;

/**
 * @brief Initialize the ext2 filesystem driver.
 */
void vxair_ext2_init(void);

/**
 * @brief Mount an ext2 filesystem.
 * @param device_node Block device containing the ext2 filesystem.
 * @return VFS node representing the root directory.
 */
vxair_vfs_node_t* vxair_ext2_mount(vxair_vfs_node_t* device_node);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_EXT2_H
