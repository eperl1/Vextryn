#pragma once
// Vextryn Air — anti-aliased TrueType-style text via host-rasterized DejaVu Sans
// glyph bitmaps (4-bit alpha), blended in the kernel with pure integer math.
// No floats, no SSE — matches the kernel's -mno-sse -mgeneral-regs-only build.
#include <stdint.h>
#include "../vxrender/vxrender.hpp"
#include "vxui_theme.hpp"
#include "dejavu_font.h"

namespace vx_text {

    // Nearest supported face (12/14/16)
    inline const VxFace* pick_face(int size) {
        int best = 12, bd = 1000;
        for (int i = 0; i < 3; i++) {
            int d = vx_fonts[i]->size - size;
            if (d < 0) d = -d;
            if (d < bd) { bd = d; best = vx_fonts[i]->size; }
        }
        const VxFace* f = &vx_face_12;
        if (best == 14) f = &vx_face_14;
        else if (best == 16) f = &vx_face_16;
        return f;
    }

    inline int text_width(int size, const char* s) {
        if (!s) return 0;
        const VxFace* f = pick_face(size);
        int pen = 0;
        for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
            unsigned char c = *p;
            if (c < f->first || c > f->last) { c = (unsigned char)'?'; }
            if (c < f->first || c > f->last) continue;
            pen += f->gm[c - f->first].adv26;
        }
        return (pen + 32) >> 6;
    }

    inline uint32_t blend_px(uint32_t bg, uint32_t fg, int a) {
        int br = (bg >> 16) & 0xFF, bg2 = (bg >> 8) & 0xFF, bb = bg & 0xFF;
        int fr = (fg >> 16) & 0xFF, fg2 = (fg >> 8) & 0xFF, fb = fg & 0xFF;
        int inv = 255 - a;
        int r = (br * inv + fr * a + 127) / 255;
        int g = (bg2 * inv + fg2 * a + 127) / 255;
        int b = (bb * inv + fb * a + 127) / 255;
        return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    // Draw text with baseline at (x, y).
    inline void draw(int x, int y, int size, const char* s, uint32_t color, uint32_t bg) {
        if (!s) return;
        const VxFace* f = pick_face(size);
        int pen = 0;
        for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
            unsigned char c = *p;
            if (c < f->first || c > f->last) c = (unsigned char)'?';
            if (c < f->first || c > f->last) continue;
            const VxGm& gm = f->gm[c - f->first];
            const uint8_t* d = f->data + gm.data_off;
            int left = x + (pen >> 6) + gm.xoff;
            int top  = y + gm.yoff;
            int stride = (gm.w + 1) >> 1;
            for (int r = 0; r < gm.h; r++) {
                int py = top + r;
                for (int c2 = 0; c2 < gm.w; c2++) {
                    uint8_t byte = d[r * stride + (c2 >> 1)];
                    int nib = (c2 & 1) ? (byte & 0x0F) : (byte >> 4);
                    if (nib == 0) continue;
                    int a = nib * 17;
                    vxr_pixel(left + c2, py, blend_px(bg, color, a));
                }
            }
            pen += gm.adv26;
        }
    }

    inline void draw_centered(int cx, int y, int size, const char* s, uint32_t color, uint32_t bg) {
        draw(cx - text_width(size, s) / 2, y, size, s, color, bg);
    }

}
