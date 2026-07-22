#include "hal_dma.h"
#include <stddef.h>

// Simple bump allocator for DMA region to satisfy compilation and basic usage.
// In a real OS, this would interact with the physical page frame allocator and virtual memory manager.
#define DMA_POOL_SIZE (1024 * 1024) // 1MB DMA pool
static uint8_t g_dma_pool[DMA_POOL_SIZE] __attribute__((aligned(4096)));
static uint32_t g_dma_offset = 0;

void vxair_hal_dma_init(void) {
    g_dma_offset = 0;
}

void* vxair_hal_dma_alloc(uint32_t size, uint32_t* physical_addr) {
    // Align size to 16 bytes
    uint32_t aligned_size = (size + 15) & ~15;
    
    if (g_dma_offset + aligned_size > DMA_POOL_SIZE) {
        return NULL; // Out of memory
    }
    
    void* virt_addr = &g_dma_pool[g_dma_offset];
    
    // In our simplified model without paging implemented in this file,
    // virtual address is assumed to correspond to some known physical mapping.
    // For the sake of this stub, we pretend virt == phys for this static array.
    if (physical_addr) {
        *physical_addr = (uint32_t)(uintptr_t)virt_addr; 
    }
    
    g_dma_offset += aligned_size;
    
    // Initialize allocated memory to zero
    uint8_t* p = (uint8_t*)virt_addr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }
    
    return virt_addr;
}

void vxair_hal_dma_free(void* virtual_addr, uint32_t size) {
    // A bump allocator cannot easily free specific blocks.
    // In a full implementation, this would return pages to the PMM.
    (void)virtual_addr;
    (void)size;
}
