#include "../include/vxair_kheap.h"
#include "../include/vxair_vmm.h"
#include "../include/vxair_pmm.h"
#include "../include/vxair_log.h"

#define SLAB_SIZES_COUNT 8
static const size_t slab_sizes[SLAB_SIZES_COUNT] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };

typedef struct slab_obj {
    struct slab_obj* next;
} slab_obj_t;

typedef struct slab_cache {
    size_t obj_size;
    slab_obj_t* free_list;
} slab_cache_t;

static slab_cache_t caches[SLAB_SIZES_COUNT];

typedef struct alloc_header {
    size_t size;
} alloc_header_t;

static void* slab_alloc(slab_cache_t* cache) {
    if (!cache->free_list) {
        vxair_paddr_t paddr = vxair_pmm_alloc_page();
        if (!paddr) return NULL;
        
        char* page = (char*)paddr;
        size_t count = VXAIR_PAGE_SIZE / cache->obj_size;
        for (size_t i = 0; i < count; i++) {
            slab_obj_t* obj = (slab_obj_t*)(page + i * cache->obj_size);
            obj->next = cache->free_list;
            cache->free_list = obj;
        }
    }
    
    slab_obj_t* obj = cache->free_list;
    cache->free_list = obj->next;
    return obj;
}

static void slab_free(slab_cache_t* cache, void* ptr) {
    slab_obj_t* obj = (slab_obj_t*)ptr;
    obj->next = cache->free_list;
    cache->free_list = obj;
}

void vxair_kheap_init(void) {
    vxair_log_info("KHeap: Initializing Slab Allocator...");
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        caches[i].obj_size = slab_sizes[i];
        caches[i].free_list = NULL;
    }
}

void* vxair_kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size_t alloc_size = size + sizeof(alloc_header_t);
    void* ptr = NULL;
    
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        if (alloc_size <= caches[i].obj_size) {
            ptr = slab_alloc(&caches[i]);
            break;
        }
    }
    
    if (!ptr) {
        size_t pages = (alloc_size + VXAIR_PAGE_SIZE - 1) / VXAIR_PAGE_SIZE;
        ptr = (void*)vxair_pmm_alloc_pages(pages);
    }
    
    if (ptr) {
        alloc_header_t* header = (alloc_header_t*)ptr;
        header->size = alloc_size;
        return (void*)((char*)ptr + sizeof(alloc_header_t));
    }
    
    return NULL;
}

void* vxair_kcalloc(size_t nitems, size_t size) {
    size_t total = nitems * size;
    void* ptr = vxair_kmalloc(total);
    if (ptr) {
        char* p = (char*)ptr;
        for (size_t i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

void* vxair_krealloc(void* ptr, size_t size) {
    if (!ptr) return vxair_kmalloc(size);
    if (size == 0) {
        vxair_kfree(ptr);
        return NULL;
    }
    
    alloc_header_t* header = (alloc_header_t*)((char*)ptr - sizeof(alloc_header_t));
    size_t old_user_size = header->size - sizeof(alloc_header_t);
    
    if (size <= old_user_size) return ptr;
    
    void* new_ptr = vxair_kmalloc(size);
    if (!new_ptr) return NULL;
    
    char* src = (char*)ptr;
    char* dst = (char*)new_ptr;
    for (size_t i = 0; i < old_user_size; i++) dst[i] = src[i];
    
    vxair_kfree(ptr);
    return new_ptr;
}

void vxair_kfree(void* ptr) {
    if (!ptr) return;
    
    alloc_header_t* header = (alloc_header_t*)((char*)ptr - sizeof(alloc_header_t));
    size_t alloc_size = header->size;
    
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        if (alloc_size <= caches[i].obj_size) {
            slab_free(&caches[i], (void*)header);
            return;
        }
    }
    
    size_t pages = (alloc_size + VXAIR_PAGE_SIZE - 1) / VXAIR_PAGE_SIZE;
    vxair_pmm_free_pages((vxair_paddr_t)header, pages);
}
