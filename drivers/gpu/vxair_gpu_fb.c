#include "vxair_gpu_fb.h"
#include <stdlib.h>
#include <string.h>

static vxair_fb_info_t g_fb_info = {0};

void vxair_gpu_fb_init(void) {
    // Simulated framebuffer initialization (e.g., 1024x768x32)
    g_fb_info.width = 1024;
    g_fb_info.height = 768;
    g_fb_info.bpp = 32;
    g_fb_info.pitch = g_fb_info.width * (g_fb_info.bpp / 8);
    
    // Allocate dummy memory for framebuffer and backbuffer
    g_fb_info.framebuffer_base = malloc(g_fb_info.pitch * g_fb_info.height);
    g_fb_info.backbuffer_base = malloc(g_fb_info.pitch * g_fb_info.height);
    
    if (g_fb_info.framebuffer_base) {
        memset(g_fb_info.framebuffer_base, 0, g_fb_info.pitch * g_fb_info.height);
    }
    if (g_fb_info.backbuffer_base) {
        memset(g_fb_info.backbuffer_base, 0, g_fb_info.pitch * g_fb_info.height);
    }
}

vxair_fb_info_t* vxair_gpu_get_fb_info(void) {
    return &g_fb_info;
}

void vxair_gpu_fb_swap_buffers(void) {
    if (g_fb_info.framebuffer_base && g_fb_info.backbuffer_base) {
        memcpy(g_fb_info.framebuffer_base, g_fb_info.backbuffer_base, g_fb_info.pitch * g_fb_info.height);
    }
}

void vxair_gpu_fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= g_fb_info.width || y >= g_fb_info.height || !g_fb_info.backbuffer_base) {
        return;
    }
    uint32_t* pixel = (uint32_t*)((uint8_t*)g_fb_info.backbuffer_base + y * g_fb_info.pitch + x * 4);
    *pixel = color;
}
