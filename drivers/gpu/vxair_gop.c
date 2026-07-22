#include "vxair_gop.h"
#include "../../boot/vxair_boot_info.h"
#include <stdint.h>
#include <stddef.h>
#include "../../kernel/core/include/vxair_vmm.h"

// Framebuffer globals
static uint32_t* fb_front = NULL;
static uint32_t* fb_back  = NULL;
static uint32_t  fb_width  = 0;
static uint32_t  fb_height = 0;
static uint32_t  fb_pitch  = 0; // bytes per row
static uint32_t  fb_bpp    = 32;
static size_t    fb_size   = 0;

// Kernel heap allocator (from slab)
extern void* vxair_kmalloc(size_t size);
extern void  vxair_kfree(void* ptr);

/**
 * @brief Initialize framebuffer from boot info
 */
void vxair_fb_init(struct vxair_boot_info* info) {
    if (!info) return;
    fb_front  = (uint32_t*)info->framebuffer.address;
    fb_width  = info->framebuffer.width;
    fb_height = info->framebuffer.height;
    fb_pitch  = info->framebuffer.pitch;
    fb_bpp    = info->framebuffer.bpp;
    fb_size   = fb_height * fb_pitch;

    extern void vxair_log_info(const char* fmt, ...);
    vxair_log_info("GOP: Framebuffer address %x, size %d", (uint32_t)(uint64_t)fb_front, (uint32_t)fb_size);
    if (!fb_front || fb_size == 0) {
        vxair_log_info("GOP: Invalid framebuffer, skipping init");
        return;
    }

    extern vxair_page_table_t* kernel_pml4;
    for (uint64_t offset = 0; offset < fb_size; offset += 4096) {
        vxair_vmm_map_page(kernel_pml4, (uint64_t)fb_front + offset, (uint64_t)fb_front + offset, 3); // 3 = PRESENT | RW
    }
    vxair_log_info("GOP: Framebuffer mapped successfully");

    // Allocate back buffer
    fb_back = (uint32_t*)vxair_kmalloc(fb_size);

    vxair_log_info("GOP: Clearing front buffer...");
    // Clear both buffers to black
    if (fb_front) {
        for (size_t i = 0; i < fb_size / 4; i++) {
            fb_front[i] = 0xFF000000;
        }
    }
    vxair_log_info("GOP: Init complete");
    if (fb_back) {
        for (size_t i = 0; i < fb_size / 4; i++) {
            fb_back[i] = 0xFF000000;
        }
    }
}

/**
 * @brief Get framebuffer dimensions
 */
uint32_t vxair_fb_get_width(void) {
    return fb_width;
}
uint32_t vxair_fb_get_height(void) {
    return fb_height;
}

/**
 * @brief Put a single pixel (writes to back buffer)
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 0xAARRGGBB format
 */
void vxair_fb_put_pixel(int32_t x,
                         int32_t y,
                         uint32_t color) {
    if (x < 0 || y < 0 || x >= (int32_t)fb_width || y >= (int32_t)fb_height) return;
    uint32_t* buf = fb_back ? fb_back : fb_front;
    if (!buf) return;

    uint32_t* line = (uint32_t*)(
        (uint8_t*)buf + y * fb_pitch);
    line[x] = color;
}

/**
 * @brief Fill rectangle with color
 */
void vxair_fb_fill_rect(int32_t x,
                          int32_t y,
                          int32_t w,
                          int32_t h,
                          uint32_t color) {
    if (x >= (int32_t)fb_width || y >= (int32_t)fb_height || x + w <= 0 || y + h <= 0) return;

    int32_t x1 = x < 0 ? 0 : x;
    int32_t y1 = y < 0 ? 0 : y;
    int32_t x2 = x + w;
    int32_t y2 = y + h;
    
    if (x2 > (int32_t)fb_width)  x2 = fb_width;
    if (y2 > (int32_t)fb_height) y2 = fb_height;

    uint32_t* buf = fb_back ? fb_back : fb_front;
    if (!buf) return;

    for (int32_t row = y1; row < y2; row++) {
        uint32_t* line = (uint32_t*)(
            (uint8_t*)buf + row * fb_pitch);
        for (int32_t col = x1; col < x2; col++) {
            line[col] = color;
        }
    }
}

/**
 * @brief Blit surface to back buffer
 */
void vxair_fb_blit(const uint32_t* src,
                    int32_t dst_x,
                    int32_t dst_y,
                    int32_t w,
                    int32_t h) {
    if (dst_x >= (int32_t)fb_width || dst_y >= (int32_t)fb_height || dst_x + w <= 0 || dst_y + h <= 0) return;
    uint32_t* buf = fb_back ? fb_back : fb_front;
    if (!buf || !src) return;

    int32_t src_y_start = dst_y < 0 ? -dst_y : 0;
    int32_t src_x_start = dst_x < 0 ? -dst_x : 0;
    int32_t dst_x1 = dst_x < 0 ? 0 : dst_x;
    int32_t dst_y1 = dst_y < 0 ? 0 : dst_y;
    int32_t dst_x2 = dst_x + w;
    int32_t dst_y2 = dst_y + h;
    
    if (dst_x2 > (int32_t)fb_width)  dst_x2 = fb_width;
    if (dst_y2 > (int32_t)fb_height) dst_y2 = fb_height;

    for (int32_t row = dst_y1; row < dst_y2; row++) {
        uint32_t* dst_line = (uint32_t*)((uint8_t*)buf + row * fb_pitch);
        const uint32_t* src_line = src + (src_y_start + (row - dst_y1)) * w;
        for (int32_t col = dst_x1; col < dst_x2; col++) {
            dst_line[col] = src_line[src_x_start + (col - dst_x1)];
        }
    }
}

/**
 * @brief Draw a character using 8x16 bitmap font
 */
static const uint8_t font8x16[128][16] = {
    // Basic ASCII font data
    // Space (32)
    [32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    // ! (33)
    [33] = {0,0x18,0x18,0x18,0x18,0x18,
            0x18,0,0x18,0x18,0,0,0,0,0,0},
    // A (65)
    [65] = {0,0,0x18,0x3C,0x66,0x66,0x7E,
            0x66,0x66,0x66,0,0,0,0,0,0},
    // B (66)
    [66] = {0,0,0x7C,0x66,0x66,0x7C,0x66,
            0x66,0x66,0x7C,0,0,0,0,0,0},
    // ... (add more as needed)
};

void vxair_font_draw_char(uint32_t x,
                           uint32_t y,
                           char ch,
                           uint32_t fg,
                           uint32_t bg) {
    uint32_t* buf = fb_back ? fb_back : fb_front;
    if (!buf) return;
    uint8_t idx = (uint8_t)ch;
    if (idx >= 128) idx = '?';
    for (int row = 0; row < 16; row++) {
        if (y + row >= fb_height) break;
        uint8_t bits = font8x16[idx][row];
        uint32_t* line = (uint32_t*)(
            (uint8_t*)buf +
            (y + row) * fb_pitch);
        for (int col = 0; col < 8; col++) {
            if (x + col >= fb_width) break;
            line[x + col] =
                (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}

void vxair_font_draw_string(uint32_t x,
                              uint32_t y,
                              const char* str,
                              uint32_t fg,
                              uint32_t bg) {
    uint32_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 16;
        } else {
            vxair_font_draw_char(
                cx, y, *str, fg, bg);
            cx += 8;
        }
        str++;
        if (cx + 8 > fb_width) {
            cx = x;
            y += 16;
        }
    }
}

/**
 * @brief Flip back buffer to front (display)
 */
void vxair_fb_flip(void) {
    if (!fb_front || !fb_back) return;
    // Fast copy using 32-bit words
    uint64_t words = fb_size / 4;
    uint32_t* src = fb_back;
    uint32_t* dst = fb_front;
    for (uint64_t i = 0; i < words; i++) {
        dst[i] = src[i];
    }
}

/**
 * @brief Clear framebuffer to color
 */
void vxair_fb_clear(uint32_t color) {
    uint32_t* buf = fb_back ? fb_back : fb_front;
    if (!buf) return;
    uint32_t total = fb_size / 4;
    for (uint32_t i = 0; i < total; i++) {
        buf[i] = color;
    }
}

/**
 * @brief Test pattern - proves fb is working
 */
void vxair_fb_test(void) {
    if (!fb_front) return;
    // Red top bar
    vxair_fb_fill_rect(0, 0,
        fb_width, 50, 0xFFFF0000);
    // Green middle
    vxair_fb_fill_rect(0, 50,
        fb_width, 50, 0xFF00FF00);
    // Blue bottom bar
    vxair_fb_fill_rect(0, 100,
        fb_width, 50, 0xFF0000FF);
    // White text
    vxair_font_draw_string(10, 10,
        "VEXTRYN AIR - DISPLAY OK",
        0xFFFFFFFF, 0xFFFF0000);
    vxair_fb_flip();
}
