#ifndef VXAIR_VFS_H
#define VXAIR_VFS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VXAIR_VFS_FLAG_FILE 0x01
#define VXAIR_VFS_FLAG_DIRECTORY 0x02
#define VXAIR_VFS_FLAG_MOUNTPOINT 0x04

typedef struct vxair_vfs_node vxair_vfs_node_t;
typedef struct vxair_dirent vxair_dirent_t;

/**
 * @brief Directory entry structure for VFS.
 */
struct vxair_dirent {
    char name[256];
    uint32_t inode;
};

/**
 * @brief Virtual File System node structure representing a file or directory.
 */
struct vxair_vfs_node {
    char name[256];
    uint32_t flags;
    uint32_t length;
    uint32_t uid;
    uint32_t gid;
    uint32_t inode;
    uint32_t impl;
    
    uint32_t (*read)(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    uint32_t (*write)(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    void (*open)(vxair_vfs_node_t *node);
    void (*close)(vxair_vfs_node_t *node);
    struct vxair_dirent *(*readdir)(vxair_vfs_node_t *node, uint32_t index);
    vxair_vfs_node_t *(*finddir)(vxair_vfs_node_t *node, char *name);
};

/**
 * @brief Initialize the Virtual File System.
 */
void vxair_vfs_init(void);

/**
 * @brief Get the root node of the VFS.
 * @return Pointer to the root VFS node.
 */
vxair_vfs_node_t* vxair_vfs_get_root(void);

/**
 * @brief Read data from a VFS node.
 * @param node Node to read from.
 * @param offset Byte offset to read from.
 * @param size Number of bytes to read.
 * @param buffer Buffer to store read data.
 * @return Number of bytes successfully read.
 */
uint32_t vxair_vfs_read(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

/**
 * @brief Write data to a VFS node.
 * @param node Node to write to.
 * @param offset Byte offset to write to.
 * @param size Number of bytes to write.
 * @param buffer Data to write.
 * @return Number of bytes successfully written.
 */
uint32_t vxair_vfs_write(vxair_vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

/**
 * @brief Open a VFS node.
 * @param node Node to open.
 */
void vxair_vfs_open(vxair_vfs_node_t *node);

/**
 * @brief Close a VFS node.
 * @param node Node to close.
 */
void vxair_vfs_close(vxair_vfs_node_t *node);

/**
 * @brief Read directory entry from a node.
 * @param node Directory node.
 * @param index Index of the entry.
 * @return Pointer to dirent structure or NULL if no more entries.
 */
struct vxair_dirent *vxair_vfs_readdir(vxair_vfs_node_t *node, uint32_t index);

/**
 * @brief Find a file in a directory node by name.
 * @param node Directory node.
 * @param name File name to look up.
 * @return Pointer to VFS node or NULL if not found.
 */
vxair_vfs_node_t *vxair_vfs_finddir(vxair_vfs_node_t *node, char *name);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_VFS_H
