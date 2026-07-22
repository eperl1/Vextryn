#include "ramdisk.h"

static vxair_vfs_node_t vxair_ramdisk_node;
static void* vxair_ramdisk_memory = NULL;
static size_t vxair_ramdisk_size = 0;

static uint32_t vxair_ramdisk_read(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    if (!vxair_ramdisk_memory || offset >= vxair_ramdisk_size) {
        return 0;
    }
    
    uint32_t bytes_to_read = size;
    if (offset + size > vxair_ramdisk_size) {
        bytes_to_read = vxair_ramdisk_size - offset;
    }
    
    uint8_t *src = (uint8_t*)vxair_ramdisk_memory + offset;
    for (uint32_t i = 0; i < bytes_to_read; ++i) {
        buffer[i] = src[i];
    }
    
    return bytes_to_read;
}

static uint32_t vxair_ramdisk_write(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    if (!vxair_ramdisk_memory || offset >= vxair_ramdisk_size) {
        return 0;
    }
    
    uint32_t bytes_to_write = size;
    if (offset + size > vxair_ramdisk_size) {
        bytes_to_write = vxair_ramdisk_size - offset;
    }
    
    uint8_t *dst = (uint8_t*)vxair_ramdisk_memory + offset;
    for (uint32_t i = 0; i < bytes_to_write; ++i) {
        dst[i] = buffer[i];
    }
    
    return bytes_to_write;
}

void vxair_ramdisk_init(void* start_address, size_t size) {
    vxair_ramdisk_memory = start_address;
    vxair_ramdisk_size = size;
    
    vxair_ramdisk_node.flags = VXAIR_VFS_FLAG_FILE;
    vxair_ramdisk_node.length = size;
    vxair_ramdisk_node.read = vxair_ramdisk_read;
    vxair_ramdisk_node.write = vxair_ramdisk_write;
    vxair_ramdisk_node.open = NULL;
    vxair_ramdisk_node.close = NULL;
    vxair_ramdisk_node.readdir = NULL;
    vxair_ramdisk_node.finddir = NULL;
}

vxair_vfs_node_t* vxair_ramdisk_get_node(void) {
    return &vxair_ramdisk_node;
}
