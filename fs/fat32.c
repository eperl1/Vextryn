#include "fat32.h"
#include <stddef.h>

static uint32_t vxair_fat32_read(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0;
}

static uint32_t vxair_fat32_write(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0;
}

static void vxair_fat32_open(vxair_vfs_node_t *node) {
    (void)node;
}

static void vxair_fat32_close(vxair_vfs_node_t *node) {
    (void)node;
}

static struct vxair_dirent *vxair_fat32_readdir(vxair_vfs_node_t *node, uint32_t index) {
    (void)node;
    (void)index;
    return NULL;
}

static vxair_vfs_node_t *vxair_fat32_finddir(vxair_vfs_node_t *node, char *name) {
    (void)node;
    (void)name;
    return NULL;
}

void vxair_fat32_init(void) {
    // Initialization code for FAT32 driver
}

vxair_vfs_node_t* vxair_fat32_mount(vxair_vfs_node_t* device_node) {
    (void)device_node;
    
    // Parse boot sector and construct root VFS node
    static vxair_vfs_node_t root;
    root.flags = VXAIR_VFS_FLAG_DIRECTORY;
    root.read = vxair_fat32_read;
    root.write = vxair_fat32_write;
    root.open = vxair_fat32_open;
    root.close = vxair_fat32_close;
    root.readdir = vxair_fat32_readdir;
    root.finddir = vxair_fat32_finddir;
    
    return &root;
}
