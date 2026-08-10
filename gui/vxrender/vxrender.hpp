// VXRender — Vextryn Air Native Graphics Layer
// The rendering foundation beneath VXUI and the compositor.
// Centralizes all drawing through a single context with clipping,
// higher-level primitives, gradients, shadows, and retained surfaces.
//
// Render flow:  Compositor → VXRender context → primitives → framebuffer
//               VXUI widgets → VXRender context → primitives → framebuffer
//
// No Apple code. No POSIX assumptions. Bare-metal framebuffer output.
#ifndef VXRENDER_HPP
#define VXRENDER_HPP

#include <stdint.h>
#include "../vxui/vxui_theme.hpp"

// ===== Forward declarations for framebuffer backend =====
extern void vxair_fb_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
extern void vxair_fb_blit(const uint32_t* src, int32_t dst_x, int32_t dst_y, int32_t w, int32_t h);
extern uint32_t vxair_fb_get_width(void);
extern uint32_t vxair_fb_get_height(void);
extern uint32_t* vxair_fb_get_backbuffer(void);
extern uint32_t vxair_fb_get_pitch(void);

// ===== Color utilities =====
namespace VxColor {
    inline uint8_t r_of(uint32_t c) { return (c >> 16) & 0xFF; }
    inline uint8_t g_of(uint32_t c) { return (c >> 8)  & 0xFF; }
    inline uint8_t b_of(uint32_t c) { return  c        & 0xFF; }
    inline uint8_t a_of(uint32_t c) { return (c >> 24) & 0xFF; }

    inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
    inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    // Linear interpolation between two colors (integer-only)
    // NOTE: differences must be computed in SIGNED arithmetic — unsigned
    // subtraction wraps when c2 channel < c1 channel (e.g. 17 - 18 = 0xFFFFFFFF),
    // which corrupts the blend into warm garbage colors.
    inline uint32_t lerp(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t) {
        if (max_t == 0) return c1;
        int32_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
        int32_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
        int32_t r = r1 + (r2 - r1) * (int32_t)t / (int32_t)max_t;
        int32_t g = g1 + (g2 - g1) * (int32_t)t / (int32_t)max_t;
        int32_t b = b1 + (b2 - b1) * (int32_t)t / (int32_t)max_t;
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    // Apply opacity to a color (returns a color with alpha channel set)
    inline uint32_t with_alpha(uint32_t c, uint8_t alpha) {
        uint32_t a = alpha;
        return (a << 24) | (c & 0xFFFFFF);
    }

    // Tint a base color with an accent (mix factor 0-100)
    inline uint32_t tint(uint32_t base, uint32_t tint_col, uint32_t factor) {
        return lerp(base, tint_col, factor, 100);
    }
}

// ===== Clip rectangle — the core of VXRender's clipping system =====
struct VxClipRect {
    int x, y, w, h;

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    // Intersect this clip rect with another, returning the result
    VxClipRect intersect(const VxClipRect& other) const {
        int ix = (x > other.x) ? x : other.x;
        int iy = (y > other.y) ? y : other.y;
        int ix2 = (x + w < other.x + other.w) ? x + w : other.x + other.w;
        int iy2 = (y + h < other.y + other.h) ? y + h : other.y + other.h;
        if (ix2 <= ix || iy2 <= iy) return {0, 0, 0, 0};
        return {ix, iy, ix2 - ix, iy2 - iy};
    }

    bool valid() const { return w > 0 && h > 0; }
};

// ===== Render Context — the central state for all drawing =====
struct VxRenderCtx {
    VxClipRect clip;
    int fb_w;
    int fb_h;

    void init() {
        fb_w = (int)vxair_fb_get_width();
        fb_h = (int)vxair_fb_get_height();
        clip = {0, 0, fb_w, fb_h};
    }

    // Push a new clip rect (intersected with current) — returns old clip for restore
    VxClipRect push_clip(int x, int y, int w, int h) {
        VxClipRect old = clip;
        VxClipRect new_clip = {x, y, w, h};
        clip = clip.intersect(new_clip);
        return old;
    }

    // Restore a previous clip rect
    void pop_clip(VxClipRect old) {
        clip = old;
    }
};

// Global render context — singleton since this is a single-head display OS
static VxRenderCtx g_vxr_ctx;

// ===== Core primitive: clipped fill rect =====
// ALL drawing in the OS ultimately goes through this function.
inline void vxr_fill_rect(int x, int y, int w, int h, uint32_t color) {
    VxClipRect& c = g_vxr_ctx.clip;
    // Clip to render context
    if (x < c.x) { w -= (c.x - x); x = c.x; }
    if (y < c.y) { h -= (c.y - y); y = c.y; }
    if (x + w > c.x + c.w) w = c.x + c.w - x;
    if (y + h > c.y + c.h) h = c.y + c.h - y;
    if (w <= 0 || h <= 0) return;
    vxair_fb_fill_rect(x, y, w, h, color);
}

// ===== Pixel (clipped) =====
inline void vxr_pixel(int x, int y, uint32_t color) {
    if (g_vxr_ctx.clip.contains(x, y))
        vxair_fb_fill_rect(x, y, 1, 1, color);
}

// ===== Blit (clipped, with source width parameter) =====
// src_w is the stride (width in pixels) of the source buffer.
// Use vxr_blit_32 for the common case of 32×32 icon blits.
inline void vxr_blit_rect(const uint32_t* src, int src_w,
                           int dst_x, int dst_y, int w, int h) {
    VxClipRect& c = g_vxr_ctx.clip;
    int sx = 0, sy = 0;
    if (dst_x < c.x) { sx = c.x - dst_x; w -= sx; dst_x = c.x; }
    if (dst_y < c.y) { sy = c.y - dst_y; h -= sy; dst_y = c.y; }
    if (dst_x + w > c.x + c.w) w = c.x + c.w - dst_x;
    if (dst_y + h > c.y + c.h) h = c.y + c.h - dst_y;
    if (w <= 0 || h <= 0) return;
    if (sx == 0 && sy == 0) {
        vxair_fb_blit(src, dst_x, dst_y, w, h);
    } else {
        for (int row = 0; row < h; row++) {
            vxair_fb_blit(src + (sy + row) * src_w + sx, dst_x, dst_y + row, w, 1);
        }
    }
}

// Convenience: blit a 32×32 source (most common case for app icons)
inline void vxr_blit(const uint32_t* src, int dst_x, int dst_y, int w, int h) {
    vxr_blit_rect(src, w, dst_x, dst_y, w, h);
}

// ===== Bordered rectangle =====
inline void vxr_rect_bordered(int x, int y, int w, int h, uint32_t fill, uint32_t border) {
    vxr_fill_rect(x, y, w, h, fill);
    vxr_fill_rect(x, y, w, 1, border);                    // top
    vxr_fill_rect(x, y + h - 1, w, 1, border);            // bottom
    vxr_fill_rect(x, y, 1, h, border);                    // left
    vxr_fill_rect(x + w - 1, y, 1, h, border);            // right
}

// ===== Rounded rectangle (full, all 4 corners) =====
inline void vxr_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if (radius <= 0) { vxr_fill_rect(x, y, w, h, color); return; }
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    // Main body
    vxr_fill_rect(x + radius, y, w - radius * 2, h, color);
    vxr_fill_rect(x, y + radius, radius, h - radius * 2, color);
    vxr_fill_rect(x + w - radius, y + radius, radius, h - radius * 2, color);
    // Corners (approximate circle quadrants)
    for (int dy = 0; dy < radius; dy++) {
        int chord = radius;
        // Simple circle: x^2 + y^2 <= r^2
        for (int dx = 0; dx < radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                // Top-left
                vxr_pixel(x + radius - 1 - dx, y + radius - 1 - dy, color);
                // Top-right
                vxr_pixel(x + w - radius + dx, y + radius - 1 - dy, color);
                // Bottom-left
                vxr_pixel(x + radius - 1 - dx, y + h - radius + dy, color);
                // Bottom-right
                vxr_pixel(x + w - radius + dx, y + h - radius + dy, color);
            }
        }
    }
}

// ===== Rounded top corners only (for windows) =====
inline void vxr_rounded_top(int x, int y, int w, int h, int radius, uint32_t color) {
    if (radius <= 0) { vxr_fill_rect(x, y, w, h, color); return; }
    if (radius > w / 2) radius = w / 2;
    // Main body (below the rounded corners)
    vxr_fill_rect(x, y + radius, w, h - radius, color);
    // Top middle strip
    vxr_fill_rect(x + radius, y, w - radius * 2, radius, color);
    // Top corners
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                vxr_pixel(x + radius - 1 - dx, y + radius - 1 - dy, color);
                vxr_pixel(x + w - radius + dx, y + radius - 1 - dy, color);
            }
        }
    }
}

// ===== Rounded rectangle outline (1px border only, NOT a fill) =====
// Draws just the outer edge of a rounded rectangle so translucent borders
// don't overpaint the surface they enclose.
inline void vxr_rounded_border(int x, int y, int w, int h, int radius, uint32_t color) {
    if (radius <= 0) { vxr_fill_rect(x, y, w, 1, color); vxr_fill_rect(x, y + h - 1, w, 1, color); vxr_fill_rect(x, y, 1, h, color); vxr_fill_rect(x + w - 1, y, 1, h, color); return; }
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    // Straight segments
    vxr_fill_rect(x + radius, y, w - radius * 2, 1, color);              // top
    vxr_fill_rect(x + radius, y + h - 1, w - radius * 2, 1, color);      // bottom
    vxr_fill_rect(x, y + radius, 1, h - radius * 2, color);              // left
    vxr_fill_rect(x + w - 1, y + radius, 1, h - radius * 2, color);      // right
    // Corner arcs (outermost filled pixel of each quadrant per row)
    for (int dy = 0; dy < radius; dy++) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= radius * radius && dx + 1 < radius) dx++;
        if (dx * dx + dy * dy > radius * radius) continue;
        // Top-left
        vxr_pixel(x + radius - 1 - dx, y + radius - 1 - dy, color);
        // Top-right
        vxr_pixel(x + w - radius + dx, y + radius - 1 - dy, color);
        // Bottom-left
        vxr_pixel(x + radius - 1 - dx, y + h - radius + dy, color);
        // Bottom-right
        vxr_pixel(x + w - radius + dx, y + h - radius + dy, color);
    }
}

// ===== Shadow (layered, soft diffusion) =====
// Uses progressively larger rounded rects to emulate a blur-like falloff.
inline void vxr_soft_shadow(int x, int y, int w, int h, int depth, int radius = 8) {
    if (depth <= 0) return;
    for (int s = depth; s >= 1; s--) {
        uint32_t alpha = 8u + (uint32_t)((depth - s + 1) * 7);
        if (alpha > 56) alpha = 56;
        uint32_t c = VxColor::with_alpha(0x000000, (uint8_t)alpha);
        int ox = x + (depth - s) / 3;
        int oy = y + (depth - s);
        int ow = w + (s * 2);
        int oh = h + (s * 2);
        int oradius = radius + (depth - s);
        vxr_rounded_rect(ox - s, oy - s, ow, oh, oradius, c);
    }
}

inline void vxr_shadow(int x, int y, int w, int h, int depth) {
    vxr_soft_shadow(x, y, w, h, depth, 8);
}

// ===== Dual-layer bevel frame =====
inline void vxr_bevel_frame(int x, int y, int w, int h, int radius, uint32_t light, uint32_t dark) {
    if (w <= 0 || h <= 0) return;
    vxr_rounded_border(x, y, w, h, radius, dark);
    vxr_fill_rect(x + 1, y + 1, w - 2, 1, light);
    vxr_fill_rect(x + 1, y + 1, 1, h - 2, light);
}

// ===== Vertical gradient fill =====
inline void vxr_gradient_v(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
    for (int row = 0; row < h; row++) {
        uint32_t c = VxColor::lerp(c1, c2, (uint32_t)row, (uint32_t)h);
        vxr_fill_rect(x, y + row, w, 1, c);
    }
}

// ===== Horizontal gradient fill =====
inline void vxr_gradient_h(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
    for (int col = 0; col < w; col++) {
        uint32_t c = VxColor::lerp(c1, c2, (uint32_t)col, (uint32_t)w);
        vxr_fill_rect(x + col, y, 1, h, c);
    }
}

// ===== Multi-stop vertical gradient =====
inline void vxr_gradient_v_multi(int x, int y, int w, int h,
                                  const uint32_t* stops, int num_stops) {
    if (num_stops < 2) {
        vxr_fill_rect(x, y, w, h, stops[0]);
        return;
    }
    int seg_h = h / (num_stops - 1);
    for (int s = 0; s < num_stops - 1; s++) {
        int sy = y + s * seg_h;
        int sh = (s == num_stops - 2) ? (y + h - sy) : seg_h;  // last segment gets remainder
        vxr_gradient_v(x, sy, w, sh, stops[s], stops[s + 1]);
    }
}

// ===== Circle (filled, clipped) =====
inline void vxr_circle(int cx, int cy, int radius, uint32_t color) {
    for (int dy = -radius; dy <= radius; dy++) {
        int half = 0;
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) half = dx;
        }
        if (half >= 0)
            vxr_fill_rect(cx - half, cy + dy, half * 2 + 1, 1, color);
    }
}

// ===== Circle outline (ring) =====
inline void vxr_circle_ring(int cx, int cy, int radius, int thickness, uint32_t color) {
    int r_out = radius;
    int r_in = radius - thickness;
    for (int dy = -r_out; dy <= r_out; dy++) {
        for (int dx = -r_out; dx <= r_out; dx++) {
            int dist = dx * dx + dy * dy;
            if (dist <= r_out * r_out && dist > r_in * r_in) {
                vxr_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

// ===== Line (Bresenham-style, clipped) =====
inline void vxr_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        vxr_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

// ===== Thick line =====
inline void vxr_line_thick(int x0, int y0, int x1, int y1, int thickness, uint32_t color) {
    int half = thickness / 2;
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        vxr_fill_rect(x0 - half, y0 - half, thickness, thickness, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

// ===== Surface — a retained offscreen buffer =====
// Surfaces allow building complex visuals in an offscreen buffer
// and blitting them as a single operation.
struct VxSurface {
    uint32_t* pixels;
    int w, h;
    bool owned;  // true if pixels were allocated by the surface

    void create(int width, int height) {
        w = width;
        h = height;
        // Simple allocation — in a real OS this would use the kernel allocator.
        // For now, surfaces use static buffers declared at call sites.
        owned = false;
        pixels = nullptr;
    }

    void blit_to(int dst_x, int dst_y) {
        if (!pixels) return;
        vxr_blit(pixels, dst_x, dst_y, w, h);
    }
};

// ===== Render Command — retained render instruction =====
// The command buffer allows building a scene as a list of commands
// that can be replayed, rather than immediate-mode drawing only.
enum VxRenderCmdType {
    VXR_CMD_FILL_RECT,
    VXR_CMD_ROUNDED_RECT,
    VXR_CMD_GRADIENT_V,
    VXR_CMD_SHADOW,
    VXR_CMD_TEXT,
    VXR_CMD_BLIT,
    VXR_CMD_CIRCLE,
    VXR_CMD_LINE,
};

struct VxRenderCmd {
    VxRenderCmdType type;
    int x, y, w, h;
    uint32_t color;
    uint32_t color2;    // for gradients
    int radius;          // for rounded rects / circles
    int depth;           // for shadows
    const char* text;    // for text commands
    const uint32_t* src; // for blits
};

// Render command buffer — a simple retained-mode scene
struct VxRenderBuffer {
    static const int MAX_COMMANDS = 256;
    VxRenderCmd commands[MAX_COMMANDS];
    int count;

    void clear() { count = 0; }

    void fill_rect(int x, int y, int w, int h, uint32_t color) {
        if (count >= MAX_COMMANDS) return;
        commands[count++] = {VXR_CMD_FILL_RECT, x, y, w, h, color, 0, 0, 0, nullptr, nullptr};
    }

    void rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
        if (count >= MAX_COMMANDS) return;
        commands[count++] = {VXR_CMD_ROUNDED_RECT, x, y, w, h, color, 0, radius, 0, nullptr, nullptr};
    }

    void gradient_v(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
        if (count >= MAX_COMMANDS) return;
        commands[count++] = {VXR_CMD_GRADIENT_V, x, y, w, h, c1, c2, 0, 0, nullptr, nullptr};
    }

    void shadow(int x, int y, int w, int h, int depth) {
        if (count >= MAX_COMMANDS) return;
        commands[count++] = {VXR_CMD_SHADOW, x, y, w, h, 0, 0, 0, depth, nullptr, nullptr};
    }

    void circle(int cx, int cy, int radius, uint32_t color) {
        if (count >= MAX_COMMANDS) return;
        commands[count++] = {VXR_CMD_CIRCLE, cx, cy, radius, 0, color, 0, radius, 0, nullptr, nullptr};
    }

    // Replay all commands through VXRender
    void replay() {
        for (int i = 0; i < count; i++) {
            VxRenderCmd& cmd = commands[i];
            switch (cmd.type) {
                case VXR_CMD_FILL_RECT:
                    vxr_fill_rect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color);
                    break;
                case VXR_CMD_ROUNDED_RECT:
                    vxr_rounded_rect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.radius, cmd.color);
                    break;
                case VXR_CMD_GRADIENT_V:
                    vxr_gradient_v(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color, cmd.color2);
                    break;
                case VXR_CMD_SHADOW:
                    vxr_shadow(cmd.x, cmd.y, cmd.w, cmd.h, cmd.depth);
                    break;
                case VXR_CMD_CIRCLE:
                    vxr_circle(cmd.x, cmd.y, cmd.radius, cmd.color);
                    break;
                default:
                    break;
            }
        }
    }
};

// ===== Convenience: scoped clip guard =====
struct VxClipGuard {
    VxClipRect old_clip;

    VxClipGuard(int x, int y, int w, int h) {
        old_clip = g_vxr_ctx.push_clip(x, y, w, h);
    }
    ~VxClipGuard() {
        g_vxr_ctx.pop_clip(old_clip);
    }
};

#endif // VXRENDER_HPP
