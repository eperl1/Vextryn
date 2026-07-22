/**
 * @file early_init.c
 * @brief Early initialization logic for Vextryn Air OS
 */
#include "vxair_boot_info.h"

static struct vxair_boot_info g_kernel_boot_info;

/* Minimal 8x8 font for basic early logging (only contains a few characters for example, filled with dummy data) */
static const uint8_t g_vxair_font_8x8[256][8] = {
    [0 ... 255] = {0, 0, 0, 0, 0, 0, 0, 0},
    ['A'] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00},
    ['S'] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['V'] = {0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18},
    ['X'] = {0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3},
    ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    ['\n']= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static uint32_t g_cursor_x = 0;
static uint32_t g_cursor_y = 0;

/**
 * @brief Draw a single character to the framebuffer
 * @param fb Framebuffer pointer
 * @param c Character to draw
 * @param color Color of the character (0xRRGGBB)
 */
static void vxair_draw_char(struct vxair_framebuffer* fb, char c, uint32_t color) {
    if (!fb || !fb->address) return;
    uint32_t* pixel_buffer = (uint32_t*)fb->address;
    
    if (c == '\n') {
        g_cursor_x = 0;
        g_cursor_y += 8;
        return;
    }
    
    for (int y = 0; y < 8; y++) {
        uint8_t row = g_vxair_font_8x8[(uint8_t)c][y];
        for (int x = 0; x < 8; x++) {
            if (row & (1 << (7 - x))) {
                pixel_buffer[(g_cursor_y + y) * (fb->pitch / 4) + (g_cursor_x + x)] = color;
            }
        }
    }
    
    g_cursor_x += 8;
    if (g_cursor_x >= fb->width) {
        g_cursor_x = 0;
        g_cursor_y += 8;
    }
}

/**
 * @brief Print a string to the framebuffer
 * @param fb Framebuffer pointer
 * @param str String to print
 */
static void vxair_print(struct vxair_framebuffer* fb, const char* str) {
    while (*str) {
        vxair_draw_char(fb, *str++, 0xFFFFFF); /* White text */
    }
}

/**
 * @brief Clear screen to a specific color
 */
static void vxair_clear_screen(struct vxair_framebuffer* fb, uint32_t color) {
    if (!fb || !fb->address) return;
    
    uint32_t* pixel_buffer = (uint32_t*)fb->address;
    for (uint32_t y = 0; y < fb->height; y++) {
        for (uint32_t x = 0; x < fb->width; x++) {
            pixel_buffer[y * (fb->pitch / 4) + x] = color;
        }
    }
    g_cursor_x = 0;
    g_cursor_y = 0;
}

/**
 * @brief Initialize the framebuffer
 * @param fb Pointer to the framebuffer structure.
 */
void vxair_init_framebuffer(struct vxair_framebuffer* fb) {
    if (!fb || !fb->address) {
        return;
    }
    
    /* Clear screen to dark grey (splash background) */
    vxair_clear_screen(fb, 0x1E1E1E);
    
    /* Setup early text rendering */
    vxair_print(fb, "VEXTRYN AIR OS - EARLY BOOT\n");
}

/* PMM Bitmap variables */
#define PMM_BITMAP_SIZE 8192
static uint8_t g_pmm_bitmap[PMM_BITMAP_SIZE]; /* Up to 256MB of memory tracked with 4KB pages in this basic bitmap */

/**
 * @brief Mark a page in the PMM bitmap as used
 */
static void vxair_pmm_mark_used(uint64_t addr) {
    uint64_t page_idx = addr / 4096;
    if (page_idx / 8 < PMM_BITMAP_SIZE) {
        g_pmm_bitmap[page_idx / 8] |= (1 << (page_idx % 8));
    }
}

/**
 * @brief Mark a page in the PMM bitmap as free
 */
static void vxair_pmm_mark_free(uint64_t addr) {
    uint64_t page_idx = addr / 4096;
    if (page_idx / 8 < PMM_BITMAP_SIZE) {
        g_pmm_bitmap[page_idx / 8] &= ~(1 << (page_idx % 8));
    }
}

/**
 * @brief Initialize the physical memory map.
 * @param mem_map Pointer to the memory map structure.
 */
void vxair_init_memory_map(struct vxair_memory_map* mem_map) {
    if (!mem_map || !mem_map->regions) {
        return;
    }
    
    /* Mark everything as used by default */
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) {
        g_pmm_bitmap[i] = 0xFF;
    }
    
    /* Parse physical memory map provided by bootloader */
    for (uint32_t i = 0; i < mem_map->num_regions; i++) {
        struct vxair_memory_region* region = &mem_map->regions[i];
        
        /* Initialize physical memory allocator (pmm) bitmaps for available regions */
        if (region->type == VXAIR_MEMMAP_TYPE_AVAILABLE) {
            for (uint64_t addr = region->base_address; addr < region->base_address + region->length; addr += 4096) {
                vxair_pmm_mark_free(addr);
            }
        }
    }
    
    /* Reserve memory for kernel, modules, and structures (e.g. first 2MB) */
    for (uint64_t addr = 0; addr < 0x200000; addr += 4096) {
        vxair_pmm_mark_used(addr);
    }
}

/**
 * @brief Initialize the boot information structure.
 * @param info Pointer to the boot information structure.
 */
void vxair_init_boot_info(struct vxair_boot_info* info) {
    if (!info) return;
    
    /* Run early inits */
    vxair_init_framebuffer(&info->framebuffer);
    vxair_init_memory_map(&info->mem_map);
    
    /* Map ACPI tables for further processing */
    if (info->rsdp_address) {
        vxair_print(&info->framebuffer, "ACPI RSDP found, ready for ACPI init.\n");
    } else {
        vxair_print(&info->framebuffer, "No ACPI RSDP found.\n");
    }
}

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved;
};

struct vxair_boot_info* vxair_early_entry(void* boot_ptr) {
    if (!boot_ptr) return &g_kernel_boot_info;
    
    uint32_t* mbd = (uint32_t*)boot_ptr;
    uint32_t total_size = mbd[0];
    
    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *)(mbd + 2);
         tag->type != 0 && (uint8_t *)tag < (uint8_t *)mbd + total_size;
         tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) 
    {
        if (tag->type == 8) { // Framebuffer tag
            struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
            g_kernel_boot_info.framebuffer.address = fb_tag->framebuffer_addr;
            g_kernel_boot_info.framebuffer.width = fb_tag->framebuffer_width;
            g_kernel_boot_info.framebuffer.height = fb_tag->framebuffer_height;
            g_kernel_boot_info.framebuffer.pitch = fb_tag->framebuffer_pitch;
            g_kernel_boot_info.framebuffer.bpp = fb_tag->framebuffer_bpp;
        }
    }
    
    return &g_kernel_boot_info;
}
