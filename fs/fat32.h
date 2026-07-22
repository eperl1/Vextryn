#ifndef VXAIR_FAT32_H
#define VXAIR_FAT32_H

#include <stdint.h>
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

/**
 * @brief FAT32 Boot Sector structure.
 */
typedef struct {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_dir_entries;
    uint16_t total_sectors_short;
    uint8_t media_descriptor;
    uint16_t fat_size_sectors;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;
    
    uint32_t fat32_size_sectors;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[420];
    uint16_t boot_sector_signature;
} vxair_fat32_boot_sector_t;

/**
 * @brief FAT32 Directory Entry structure.
 */
typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t cluster_low;
    uint32_t file_size;
} vxair_fat32_dir_entry_t;

#pragma pack(pop)

/**
 * @brief Initialize the FAT32 filesystem driver.
 */
void vxair_fat32_init(void);

/**
 * @brief Mount a FAT32 filesystem.
 * @param device_node Block device containing the FAT32 filesystem.
 * @return VFS node representing the root directory.
 */
vxair_vfs_node_t* vxair_fat32_mount(vxair_vfs_node_t* device_node);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_FAT32_H
