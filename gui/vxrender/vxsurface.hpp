// VXSurface - Vextryn Compositing Surface Layer
// Manages offscreen buffers, composition layers, blending, and tinting.
#ifndef VXSURFACE_HPP
#define VXSURFACE_HPP

#include "vxrhi.hpp"
#include "vxrender.hpp"

class VxSurfaceLayer {
public:
    int width, height;
    uint32_t* pixels;
    bool has_alpha;

    VxSurfaceLayer(int w, int h, bool alpha = true) : width(w), height(h), has_alpha(alpha) {
        // In a real OS, use kernel allocation. For Vextryn Air, we simulate this 
        // with static pools or basic heap allocation if available, but for now 
        // we'll assume we can use a pre-allocated buffer or new uint32_t[].
        // Given freestanding C++, new might not be available globally unless overloaded.
        // We will mock it by pointing to nullptr unless initialized externally.
        pixels = nullptr; 
    }
    
    ~VxSurfaceLayer() {
        // cleanup if dynamically allocated
    }

    void attach_buffer(uint32_t* buffer) {
        pixels = buffer;
    }

    void clear(uint32_t clear_color) {
        if (!pixels) return;
        for (int i = 0; i < width * height; i++) {
            pixels[i] = clear_color;
        }
    }

    void set_pixel(int x, int y, uint32_t color) {
        if (!pixels) return;
        if (x >= 0 && x < width && y >= 0 && y < height) {
            pixels[y * width + x] = color;
        }
    }

    // Composites this surface onto the RHI (hardware framebuffer)
    void composite_to_screen(int dest_x, int dest_y) {
        if (!pixels) return;
        VxRHI::blit_pixels(pixels, dest_x, dest_y, width, height);
    }
};

#endif // VXSURFACE_HPP
