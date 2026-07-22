#include "../include/vxair_pmm.h"
#include "../include/vxair_log.h"

#define PMM_MAX_PAGES (1024 * 1024 * 4) // Support up to 16GB
#define PMM_BITMAP_SIZE (PMM_MAX_PAGES / 8)

static uint8_t pmm_bitmap[PMM_BITMAP_SIZE];
static size_t pmm_total_memory = 0;
static size_t pmm_free_memory = 0;
static size_t pmm_first_free = 0;

static inline void bitmap_set(size_t bit) { pmm_bitmap[bit / 8] |= (1 << (bit % 8)); }
static inline void bitmap_clear(size_t bit) { pmm_bitmap[bit / 8] &= ~(1 << (bit % 8)); }
static inline bool bitmap_test(size_t bit) { return pmm_bitmap[bit / 8] & (1 << (bit % 8)); }

extern char __kernel_end;

void vxair_pmm_init(void* mmap_info) {
    vxair_log_info("PMM: Initializing...");
    (void)mmap_info;
    
    size_t mem_size = 256 * 1024 * 1024;
    pmm_total_memory = mem_size;
    pmm_free_memory = mem_size;
    
    for (size_t i = 0; i < PMM_BITMAP_SIZE; i++) pmm_bitmap[i] = 0;
    
    // Reserve up to __kernel_end (physical)
    uint64_t kernel_end_phys = (uint64_t)&__kernel_end - 0xFFFFFFFF80000000ull;
    size_t reserved_pages = (kernel_end_phys + VXAIR_PAGE_SIZE - 1) / VXAIR_PAGE_SIZE;
    
    // Also reserve the first 1MB just in case
    if (reserved_pages < 256) reserved_pages = 256;
    
    for (size_t i = 0; i < reserved_pages; i++) {
        bitmap_set(i);
        pmm_free_memory -= VXAIR_PAGE_SIZE;
    }
    pmm_first_free = reserved_pages;
}

vxair_paddr_t vxair_pmm_alloc_page(void) {
    for (size_t i = pmm_first_free; i < PMM_MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_free_memory -= VXAIR_PAGE_SIZE;
            pmm_first_free = i + 1;
            return (vxair_paddr_t)(i * VXAIR_PAGE_SIZE);
        }
    }
    vxair_log_error("PMM: Out of memory!");
    return 0;
}

vxair_paddr_t vxair_pmm_alloc_pages(size_t count) {
    if (count == 0) return 0;
    size_t free_count = 0, start_idx = 0;
    for (size_t i = pmm_first_free; i < PMM_MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            if (free_count == 0) start_idx = i;
            free_count++;
            if (free_count == count) {
                for (size_t j = start_idx; j < start_idx + count; j++) bitmap_set(j);
                pmm_free_memory -= count * VXAIR_PAGE_SIZE;
                return (vxair_paddr_t)(start_idx * VXAIR_PAGE_SIZE);
            }
        } else {
            free_count = 0;
        }
    }
    vxair_log_error("PMM: Out of contiguous memory!");
    return 0;
}

void vxair_pmm_free_page(vxair_paddr_t page) {
    size_t idx = page / VXAIR_PAGE_SIZE;
    if (bitmap_test(idx)) {
        bitmap_clear(idx);
        pmm_free_memory += VXAIR_PAGE_SIZE;
        if (idx < pmm_first_free) pmm_first_free = idx;
    }
}

void vxair_pmm_free_pages(vxair_paddr_t page, size_t count) {
    for (size_t i = 0; i < count; i++) vxair_pmm_free_page(page + i * VXAIR_PAGE_SIZE);
}

size_t vxair_pmm_get_free_memory(void) { return pmm_free_memory; }
size_t vxair_pmm_get_total_memory(void) { return pmm_total_memory; }
