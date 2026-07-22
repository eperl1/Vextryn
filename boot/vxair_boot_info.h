#ifndef VXAIR_BOOT_INFO_H
#define VXAIR_BOOT_INFO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory region types
 */
#define VXAIR_MEMMAP_TYPE_AVAILABLE 1
#define VXAIR_MEMMAP_TYPE_RESERVED  2
#define VXAIR_MEMMAP_TYPE_ACPI_RECLAIM 3
#define VXAIR_MEMMAP_TYPE_ACPI_NVS 4
#define VXAIR_MEMMAP_TYPE_BAD_MEMORY 5

/**
 * @brief Represents a single physical memory region.
 */
struct vxair_memory_region {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;
};

/**
 * @brief Represents the memory map of the system.
 */
struct vxair_memory_map {
    uint32_t num_regions;
    struct vxair_memory_region* regions;
};

/**
 * @brief Represents the system framebuffer.
 */
struct vxair_framebuffer {
    uint64_t address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
};

/**
 * @brief Boot information structure passed from bootloader to the kernel.
 */
struct vxair_boot_info {
    struct vxair_memory_map mem_map;
    struct vxair_framebuffer framebuffer;
    uint64_t rsdp_address;
    uint64_t kernel_physical_base;
    uint64_t kernel_virtual_base;
};

/**
 * @brief Initialize the boot information structure.
 * @param info Pointer to the boot information structure.
 */
void vxair_init_boot_info(struct vxair_boot_info* info);

/**
 * @brief Initialize the framebuffer.
 * @param fb Pointer to the framebuffer structure.
 */
void vxair_init_framebuffer(struct vxair_framebuffer* fb);

/**
 * @brief Initialize the physical memory map.
 * @param mem_map Pointer to the memory map structure.
 */
void vxair_init_memory_map(struct vxair_memory_map* mem_map);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BOOT_INFO_H
