#pragma once
#include "vxair_types.h"

#define VXAIR_PAGE_SIZE 4096

/**
 * @file vxair_pmm.h
 * @brief Physical Memory Manager
 */
void vxair_pmm_init(void* mmap_info);
vxair_paddr_t vxair_pmm_alloc_page(void);
vxair_paddr_t vxair_pmm_alloc_pages(size_t count);
void vxair_pmm_free_page(vxair_paddr_t page);
void vxair_pmm_free_pages(vxair_paddr_t page, size_t count);
size_t vxair_pmm_get_free_memory(void);
size_t vxair_pmm_get_total_memory(void);
