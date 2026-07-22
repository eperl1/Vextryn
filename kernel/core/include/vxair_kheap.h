#pragma once
#include "vxair_types.h"

/**
 * @file vxair_kheap.h
 * @brief Kernel Heap Allocator (Slab Allocator)
 */

// Kernel Heap Allocator
void vxair_kheap_init(void);
void* vxair_kmalloc(size_t size);
void* vxair_kcalloc(size_t nitems, size_t size);
void* vxair_krealloc(void* ptr, size_t size);
void vxair_kfree(void* ptr);
