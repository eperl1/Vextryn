// VXRHI - Vextryn Render Hardware Interface
// Abstracts the raw framebuffer backend.
// Currently wraps vxair_fb_*, but designed to support future GPU acceleration.
#ifndef VXRHI_HPP
#define VXRHI_HPP

#include <stdint.h>
#include <stddef.h>

// Extern definitions expected from the kernel/platform layer
extern void vxair_fb_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
extern void vxair_fb_blit(const uint32_t* src, int32_t dst_x, int32_t dst_y, int32_t w, int32_t h);
extern uint32_t vxair_fb_get_width(void);
extern uint32_t vxair_fb_get_height(void);
extern uint32_t* vxair_fb_get_buffer(void);

class VxRHI {
public:
    static int width() { return (int)vxair_fb_get_width(); }
    static int height() { return (int)vxair_fb_get_height(); }
    static uint32_t* get_buffer() { return vxair_fb_get_buffer(); }

    static void fill_rect(int x, int y, int w, int h, uint32_t color) {
        vxair_fb_fill_rect(x, y, w, h, color);
    }
    
    static void blit_pixels(const uint32_t* src, int dst_x, int dst_y, int w, int h) {
        vxair_fb_blit(src, dst_x, dst_y, w, h);
    }
};

#endif // VXRHI_HPP
