#pragma once
#include "vxair_types.h"

/**
 * @file vxair_vmm.h
 * @brief Virtual Memory Manager
 */

// Flags for page mapping
#define VXAIR_VMM_PRESENT  (1 << 0)
#define VXAIR_VMM_RW       (1 << 1)
#define VXAIR_VMM_USER     (1 << 2)

typedef struct vxair_page_table {
    uint64_t entries[512];
} vxair_page_table_t;

// Virtual Memory Manager
void vxair_vmm_init(void);
vxair_page_table_t* vxair_vmm_create_address_space(void);
void vxair_vmm_switch_address_space(vxair_page_table_t* pml4);
bool vxair_vmm_map_page(vxair_page_table_t* pt, vxair_vaddr_t vaddr, vxair_paddr_t paddr, uint64_t flags);
void vxair_vmm_unmap_page(vxair_page_table_t* pt, vxair_vaddr_t vaddr);
