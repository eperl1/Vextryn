#ifndef VXAIR_GPU_FB_H
#define VXAIR_GPU_FB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Framebuffer information structure
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    void* framebuffer_base;
    void* backbuffer_base;
} vxair_fb_info_t;

/**
 * @brief Initialize the framebuffer
 */
void vxair_gpu_fb_init(void);

/**
 * @brief Get the framebuffer information
 * @return Pointer to framebuffer info
 */
vxair_fb_info_t* vxair_gpu_get_fb_info(void);

/**
 * @brief Swap back buffer to front buffer
 */
void vxair_gpu_fb_swap_buffers(void);

typedef struct {
    int32_t x, y, w, h;
} vxair_rect_t;

/**
 * @brief Draw a pixel to the back buffer
 * @param x X coordinate
 * @param y Y coordinate
 * @param color ARGB color
 */
void vxair_gpu_fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);

/**
 * @brief Present specific dirty rectangle from back buffer to front buffer
 */
void vxair_gpu_fb_present_rect(int32_t x, int32_t y, int32_t w, int32_t h);

/**
 * @brief Present multiple dirty rectangles
 */
void vxair_gpu_fb_present_rects(const vxair_rect_t* rects, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_GPU_FB_H
