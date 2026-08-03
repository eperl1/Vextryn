// VXSurface — Vextryn Offscreen Surface, Blending & Damage Tracking Pipeline
#ifndef VXSURFACE_HPP
#define VXSURFACE_HPP

#include <stdint.h>
#include <stddef.h>
#include "../../drivers/gpu/vxair_gpu_fb.h"
#include "vxrhi.hpp"

// Hardware screen dimensions helper
extern uint32_t vxair_fb_get_width(void);
extern uint32_t vxair_fb_get_height(void);
extern void vxair_fb_blit(const uint32_t* src, int32_t dst_x, int32_t dst_y, int32_t w, int32_t h);

// Rect structure
struct VxRect {
    int x, y, w, h;

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const VxRect& o) const {
        return !(x + w <= o.x || o.x + o.w <= x || y + h <= o.y || o.y + o.h <= y);
    }

    VxRect intersect(const VxRect& o) const {
        int ix1 = (x > o.x) ? x : o.x;
        int iy1 = (y > o.y) ? y : o.y;
        int ix2 = (x + w < o.x + o.w) ? x + w : o.x + o.w;
        int iy2 = (y + h < o.y + o.h) ? y + h : o.y + o.h;
        if (ix2 <= ix1 || iy2 <= iy1) return {0, 0, 0, 0};
        return {ix1, iy1, ix2 - ix1, iy2 - iy1};
    }

    bool valid() const { return w > 0 && h > 0; }
};

// Fast ARGB Alpha Blending (src OVER dst)
static inline uint32_t vx_blend_argb(uint32_t src, uint32_t dst) {
    uint32_t sa = (src >> 24) & 0xFF;
    if (sa == 255) return src;
    if (sa == 0) return dst;

    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8) & 0xFF;
    uint32_t sb = src & 0xFF;

    uint32_t da = (dst >> 24) & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;

    uint32_t inv_sa = 255 - sa;
    uint32_t out_r = (sr * sa + dr * inv_sa) / 255;
    uint32_t out_g = (sg * sa + dg * inv_sa) / 255;
    uint32_t out_b = (sb * sa + db * inv_sa) / 255;
    uint32_t out_a = sa + (da * inv_sa) / 255;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

// Damage Region Tracker for dirty rectangle invalidation
struct VxDamageTracker {
    static const int MAX_DIRTY_RECTS = 32;
    vxair_rect_t rects[MAX_DIRTY_RECTS];
    int count;

    void reset() { count = 0; }

    void add(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) return;
        if (count < MAX_DIRTY_RECTS) {
            rects[count++] = {x, y, w, h};
        } else {
            // Merge into overall bounding box if pool is full
            int x1 = rects[0].x, y1 = rects[0].y;
            int x2 = x1 + rects[0].w, y2 = y1 + rects[0].h;
            for (int i = 1; i < count; i++) {
                if (rects[i].x < x1) x1 = rects[i].x;
                if (rects[i].y < y1) y1 = rects[i].y;
                if (rects[i].x + rects[i].w > x2) x2 = rects[i].x + rects[i].w;
                if (rects[i].y + rects[i].h > y2) y2 = rects[i].y + rects[i].h;
            }
            if (x < x1) x1 = x;
            if (y < y1) y1 = y;
            if (x + w > x2) x2 = x + w;
            if (y + h > y2) y2 = y + h;
            rects[0] = {x1, y1, x2 - x1, y2 - y1};
            count = 1;
        }
    }
};

// Retained Offscreen Surface backing store for windows & components
class VxSurfaceLayer {
public:
    int width;
    int height;
    uint32_t* pixels;
    bool has_alpha;
    bool dirty;
    VxRect damage_rect;

    VxSurfaceLayer()
        : width(0), height(0), pixels(nullptr), has_alpha(true), dirty(true), damage_rect({0, 0, 0, 0}) {}

    VxSurfaceLayer(int w, int h, uint32_t* buffer = nullptr, bool alpha = true)
        : width(w), height(h), pixels(buffer), has_alpha(alpha), dirty(true), damage_rect({0, 0, w, h}) {}

    void init(int w, int h, uint32_t* buffer, bool alpha = true) {
        width = w;
        height = h;
        pixels = buffer;
        has_alpha = alpha;
        dirty = true;
        damage_rect = {0, 0, w, h};
    }

    void mark_dirty(int x = 0, int y = 0, int w = -1, int h = -1) {
        if (w < 0) w = width;
        if (h < 0) h = height;
        dirty = true;
        if (damage_rect.w == 0 || damage_rect.h == 0) {
            damage_rect = {x, y, w, h};
        } else {
            int x1 = damage_rect.x < x ? damage_rect.x : x;
            int y1 = damage_rect.y < y ? damage_rect.y : y;
            int x2 = (damage_rect.x + damage_rect.w > x + w) ? (damage_rect.x + damage_rect.w) : (x + w);
            int y2 = (damage_rect.y + damage_rect.h > y + h) ? (damage_rect.y + damage_rect.h) : (y + h);
            damage_rect = {x1, y1, x2 - x1, y2 - y1};
        }
    }

    void clear(uint32_t clear_color) {
        if (!pixels) return;
        for (int i = 0; i < width * height; i++) {
            pixels[i] = clear_color;
        }
        mark_dirty(0, 0, width, height);
    }

    void fill_rect(int x, int y, int w, int h, uint32_t color) {
        if (!pixels) return;
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > width) w = width - x;
        if (y + h > height) h = height - y;
        if (w <= 0 || h <= 0) return;

        bool is_blend = ((color >> 24) & 0xFF) < 255;
        for (int r = 0; r < h; r++) {
            uint32_t* row = pixels + (y + r) * width + x;
            for (int c = 0; c < w; c++) {
                row[c] = is_blend ? vx_blend_argb(color, row[c]) : color;
            }
        }
        mark_dirty(x, y, w, h);
    }

    void set_pixel(int x, int y, uint32_t color) {
        if (!pixels || x < 0 || x >= width || y < 0 || y >= height) return;
        pixels[y * width + x] = ((color >> 24) & 0xFF) < 255
            ? vx_blend_argb(color, pixels[y * width + x])
            : color;
        mark_dirty(x, y, 1, 1);
    }

    // Composite sub-rect of this surface onto a target surface buffer
    void blend_to(uint32_t* dst_buffer, int dst_w, int dst_h, int dst_x, int dst_y) const {
        if (!pixels || !dst_buffer) return;
        int sx = 0, sy = 0;
        int w = width, h = height;

        if (dst_x < 0) { sx = -dst_x; w += dst_x; dst_x = 0; }
        if (dst_y < 0) { sy = -dst_y; h += dst_y; dst_y = 0; }
        if (dst_x + w > dst_w) w = dst_w - dst_x;
        if (dst_y + h > dst_h) h = dst_h - dst_y;
        if (w <= 0 || h <= 0) return;

        for (int r = 0; r < h; r++) {
            const uint32_t* src_row = pixels + (sy + r) * width + sx;
            uint32_t* dst_row = dst_buffer + (dst_y + r) * dst_w + dst_x;
            for (int c = 0; c < w; c++) {
                dst_row[c] = has_alpha ? vx_blend_argb(src_row[c], dst_row[c]) : src_row[c];
            }
        }
    }

    // Direct hardware blit to GOP backbuffer
    void composite_to_screen(int dest_x, int dest_y) {
        if (!pixels) return;
        vxair_fb_blit(pixels, dest_x, dest_y, width, height);
    }
};

#endif // VXSURFACE_HPP
