#include "../include/vxair_vmm.h"
#include "../include/vxair_pmm.h"
#include "../include/vxair_log.h"

#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ull

vxair_page_table_t* kernel_pml4;

static void* paddr_to_vaddr(vxair_paddr_t paddr) { return (void*)paddr; }

static inline void invlpg(vxair_vaddr_t vaddr) {
    __asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline void set_cr3(vxair_paddr_t pml4_addr) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4_addr) : "memory");
}

void vxair_vmm_init(void) {
    vxair_log_info("VMM: Initializing...");
    kernel_pml4 = (vxair_page_table_t*)paddr_to_vaddr(vxair_pmm_alloc_page());
    
    for (int i = 0; i < 512; i++) kernel_pml4->entries[i] = 0;
    
    // Map first 1GB identity and higher half
    for (uint64_t i = 0; i < 1024 * 256; i++) {
        vxair_vmm_map_page(kernel_pml4, i * VXAIR_PAGE_SIZE, i * VXAIR_PAGE_SIZE, VXAIR_VMM_PRESENT | VXAIR_VMM_RW);
        vxair_vmm_map_page(kernel_pml4, 0xFFFFFFFF80000000ull + i * VXAIR_PAGE_SIZE, i * VXAIR_PAGE_SIZE, VXAIR_VMM_PRESENT | VXAIR_VMM_RW);
    }
    
    set_cr3((vxair_paddr_t)kernel_pml4);
}

vxair_page_table_t* vxair_vmm_create_address_space(void) {
    vxair_page_table_t* pml4 = (vxair_page_table_t*)paddr_to_vaddr(vxair_pmm_alloc_page());
    for (int i = 0; i < 512; i++) pml4->entries[i] = 0;
    
    for (int i = 256; i < 512; i++) {
        pml4->entries[i] = kernel_pml4->entries[i];
    }
    return pml4;
}

void vxair_vmm_switch_address_space(vxair_page_table_t* pml4) {
    set_cr3((vxair_paddr_t)pml4);
}

static vxair_page_table_t* get_next_level(vxair_page_table_t* pt, uint32_t index, bool allocate, uint64_t flags) {
    if (pt->entries[index] & VXAIR_VMM_PRESENT) {
        return (vxair_page_table_t*)paddr_to_vaddr(pt->entries[index] & PTE_ADDR_MASK);
    }
    if (!allocate) return NULL;
    
    vxair_paddr_t new_pt_paddr = vxair_pmm_alloc_page();
    if (!new_pt_paddr) return NULL;
    
    vxair_page_table_t* new_pt = (vxair_page_table_t*)paddr_to_vaddr(new_pt_paddr);
    for (int i = 0; i < 512; i++) new_pt->entries[i] = 0;
    
    pt->entries[index] = new_pt_paddr | flags | VXAIR_VMM_PRESENT;
    return new_pt;
}

bool vxair_vmm_map_page(vxair_page_table_t* pml4, vxair_vaddr_t vaddr, vxair_paddr_t paddr, uint64_t flags) {
    uint32_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint32_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint32_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint32_t pt_idx   = (vaddr >> 12) & 0x1FF;
    
    vxair_page_table_t* pdpt = get_next_level(pml4, pml4_idx, true, VXAIR_VMM_RW | VXAIR_VMM_USER);
    if (!pdpt) return false;
    
    vxair_page_table_t* pd = get_next_level(pdpt, pdpt_idx, true, VXAIR_VMM_RW | VXAIR_VMM_USER);
    if (!pd) return false;
    
    vxair_page_table_t* pt = get_next_level(pd, pd_idx, true, VXAIR_VMM_RW | VXAIR_VMM_USER);
    if (!pt) return false;
    
    pt->entries[pt_idx] = paddr | flags | VXAIR_VMM_PRESENT;
    invlpg(vaddr);
    return true;
}

void vxair_vmm_unmap_page(vxair_page_table_t* pml4, vxair_vaddr_t vaddr) {
    uint32_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint32_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint32_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint32_t pt_idx   = (vaddr >> 12) & 0x1FF;
    
    vxair_page_table_t* pdpt = get_next_level(pml4, pml4_idx, false, 0);
    if (!pdpt) return;
    
    vxair_page_table_t* pd = get_next_level(pdpt, pdpt_idx, false, 0);
    if (!pd) return;
    
    vxair_page_table_t* pt = get_next_level(pd, pd_idx, false, 0);
    if (!pt) return;
    
    pt->entries[pt_idx] = 0;
    invlpg(vaddr);
}
