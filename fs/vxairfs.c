#include "vxairfs.h"
#include <stddef.h>

#define VXAIRFS_MAGIC 0x56584152 // 'VXAR'

static vxair_vfs_node_t* vxair_vxairfs_root_node = NULL;
static vxair_vxairfs_superblock_t vxairfs_sb;

static uint32_t vxair_vxairfs_read(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0; 
}

static uint32_t vxair_vxairfs_write(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0; 
}

static void vxair_vxairfs_open(vxair_vfs_node_t *node) {
    (void)node;
}

static void vxair_vxairfs_close(vxair_vfs_node_t *node) {
    (void)node;
}

static struct vxair_dirent *vxair_vxairfs_readdir(vxair_vfs_node_t *node, uint32_t index) {
    (void)node;
    (void)index;
    return NULL; 
}

static vxair_vfs_node_t *vxair_vxairfs_finddir(vxair_vfs_node_t *node, char *name) {
    (void)node;
    (void)name;
    return NULL; 
}

void vxair_vxairfs_init(void) {
    vxairfs_sb.magic = VXAIRFS_MAGIC;
    vxairfs_sb.block_size = 4096;
    vxairfs_sb.num_blocks = 0;
    vxairfs_sb.num_inodes = 0;
    vxairfs_sb.free_blocks = 0;
    vxairfs_sb.free_inodes = 0;
    vxairfs_sb.root_inode = 1;
}

vxair_vfs_node_t* vxair_vxairfs_mount(vxair_vfs_node_t* device_node) {
    (void)device_node;
    if (vxair_vxairfs_root_node == NULL) {
        return NULL;
    }
    
    vxair_vxairfs_root_node->flags = VXAIR_VFS_FLAG_DIRECTORY;
    vxair_vxairfs_root_node->read = vxair_vxairfs_read;
    vxair_vxairfs_root_node->write = vxair_vxairfs_write;
    vxair_vxairfs_root_node->open = vxair_vxairfs_open;
    vxair_vxairfs_root_node->close = vxair_vxairfs_close;
    vxair_vxairfs_root_node->readdir = vxair_vxairfs_readdir;
    vxair_vxairfs_root_node->finddir = vxair_vxairfs_finddir;
    
    return vxair_vxairfs_root_node;
}
