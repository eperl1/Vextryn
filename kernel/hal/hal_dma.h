#ifndef VXAIR_HAL_DMA_H
#define VXAIR_HAL_DMA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DMA memory allocator subsystem
 */
void vxair_hal_dma_init(void);

/**
 * @brief Allocate contiguous physical memory suitable for DMA
 * @param size Size in bytes to allocate
 * @param physical_addr Pointer to store the resulting physical address
 * @return Virtual address pointer to the allocated memory, or NULL on failure
 */
void* vxair_hal_dma_alloc(uint32_t size, uint32_t* physical_addr);

/**
 * @brief Free previously allocated DMA memory
 * @param virtual_addr Virtual address of the memory
 * @param size Size in bytes that was allocated
 */
void vxair_hal_dma_free(void* virtual_addr, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_DMA_H
