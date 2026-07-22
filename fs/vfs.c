#include "vfs.h"
#include <stddef.h>

static vxair_vfs_node_t *vxair_vfs_root = NULL;

void vxair_vfs_init(void) {
    vxair_vfs_root = NULL;
}

vxair_vfs_node_t* vxair_vfs_get_root(void) {
    return vxair_vfs_root;
}

uint32_t vxair_vfs_read(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node != NULL && node->read != NULL) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vxair_vfs_write(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node != NULL && node->write != NULL) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vxair_vfs_open(vxair_vfs_node_t *node) {
    if (node != NULL && node->open != NULL) {
        node->open(node);
    }
}

void vxair_vfs_close(vxair_vfs_node_t *node) {
    if (node != NULL && node->close != NULL) {
        node->close(node);
    }
}

struct vxair_dirent *vxair_vfs_readdir(vxair_vfs_node_t *node, uint32_t index) {
    if (node != NULL && (node->flags & VXAIR_VFS_FLAG_DIRECTORY) && node->readdir != NULL) {
        return node->readdir(node, index);
    }
    return NULL;
}

vxair_vfs_node_t *vxair_vfs_finddir(vxair_vfs_node_t *node, char *name) {
    if (node != NULL && (node->flags & VXAIR_VFS_FLAG_DIRECTORY) && node->finddir != NULL) {
        return node->finddir(node, name);
    }
    return NULL;
}
