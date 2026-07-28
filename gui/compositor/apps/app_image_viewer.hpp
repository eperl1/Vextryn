#pragma once
// V5 Image Viewer — VXUI-themed gallery with 3 procedural images
#include <stdint.h>

static void draw_app_image_viewer(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int ax = w.x + 16;
    int ay = w.y + 48;
    int aw = w.w - 32;
    int ah = w.h - 66;
    if (aw <= 0 || ah <= 0) return;
    uint32_t accent = VxTheme::accent();

    static int current_image = 0;

    // Image area
    int img_x = ax + 10;
    int img_y = ay + 10;
    int img_w = aw - 20;
    int img_h = ah - 56;
    if (img_w <= 20 || img_h <= 20) return;

    // Frame border
    vxair_fb_fill_rect(img_x - 2, img_y - 2, img_w + 4, 2, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(img_x - 2, img_y + img_h, img_w + 4, 2, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(img_x - 2, img_y, 2, img_h, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(img_x + img_w, img_y, 2, img_h, VxTheme::BORDER_STRONG);

    // Draw image content
    if (current_image == 0) {
        // Sunset over mountains — premium palette
        for (int y = 0; y < img_h; y++) {
            uint32_t sky = lerp_color(0xFF1A1A2E, 0xFFE94560, y, img_h / 2);
            if (y > img_h / 2) sky = lerp_color(0xFFE94560, 0xFF0F3460, y - img_h / 2, img_h / 2);
            vxair_fb_fill_rect(img_x, img_y + y, img_w, 1, sky);
        }
        // Sun
        int sun_cx = img_x + img_w / 2;
        int sun_cy = img_y + img_h / 3;
        int sun_r = img_w / 8;
        for (int dy = -sun_r; dy <= sun_r; dy++) {
            int half = 0;
            for (int dx = -sun_r; dx <= sun_r; dx++) {
                if (dx * dx + dy * dy <= sun_r * sun_r) half = dx;
            }
            if (half >= 0)
                vxair_fb_fill_rect(sun_cx - half, sun_cy + dy, half * 2 + 1, 1, 0xFFFBBF24);
        }
        // Mountain silhouettes
        for (int row = 0; row < img_h / 3; row++) {
            int w1 = row * 2;
            vxair_fb_fill_rect(sun_cx - w1 - img_w / 6, img_y + img_h - row, w1 * 2, 1, 0xFF16213E);
            int w2 = row * 3 / 2;
            vxair_fb_fill_rect(sun_cx + img_w / 4 - w2, img_y + img_h - row, w2 * 2, 1, 0xFF0F3460);
        }
    } else if (current_image == 1) {
        // Abstract gradient art — blue/purple waves
        for (int y = 0; y < img_h; y++) {
            uint32_t c = lerp_color(0xFF0F0C29, 0xFF302B63, y, img_h);
            c = lerp_color(c, 0xFF24243E, y * 2, img_h * 3);
            vxair_fb_fill_rect(img_x, img_y + y, img_w, 1, c);
        }
        // Accent-colored wave circles
        for (int i = 0; i < 5; i++) {
            int cx = img_x + img_w * (i + 1) / 6;
            int cy = img_y + img_h / 2 + (i % 2 ? 20 : -20);
            int r = 20 + i * 8;
            for (int dy = -r; dy <= r; dy++) {
                int half = 0;
                for (int dx = -r; dx <= r; dx++) {
                    if (dx * dx + dy * dy <= r * r && dx * dx + dy * dy > (r - 4) * (r - 4)) half = dx;
                }
                if (half >= 0)
                    vxair_fb_fill_rect(cx - half, cy + dy, half * 2 + 1, 1, accent);
            }
        }
    } else {
        // Geometric city skyline at night
        for (int y = 0; y < img_h; y++) {
            uint32_t c = lerp_color(0xFF080A10, 0xFF1A1A2E, y, img_h);
            vxair_fb_fill_rect(img_x, img_y + y, img_w, 1, c);
        }
        // Buildings
        int bld_w = img_w / 8;
        for (int b = 0; b < 7; b++) {
            int bx = img_x + b * bld_w + 4;
            int bh = (b * 37 + 80) % (img_h * 3 / 4) + img_h / 4;
            int by = img_y + img_h - bh;
            vxair_fb_fill_rect(bx, by, bld_w - 8, bh, VxTheme::SURFACE_HIGH);
            // Windows
            for (int wy = by + 8; wy < img_y + img_h - 8; wy += 16) {
                for (int wx = bx + 6; wx < bx + bld_w - 14; wx += 12) {
                    if (((wx + wy + b) % 3) == 0)
                        vxair_fb_fill_rect(wx, wy, 4, 6, accent);
                }
            }
        }
    }

    // Image counter
    char counter[8];
    counter[0] = '0' + current_image + 1; counter[1] = '/'; counter[2] = '3'; counter[3] = 0;
    for (int i = 0; counter[i]; i++)
        draw_abstract_char(ax + aw / 2 - 12 + i * 10, ay + img_h + 22, counter[i], VxTheme::TEXT_SECONDARY);

    // Navigation buttons
    int btn_w = 70, btn_h = 28;
    int prev_x = ax + aw / 2 - btn_w - 20;
    int next_x = ax + aw / 2 + 20;
    int btn_y = ay + img_h + 16;

    bool prev_hover = (mouse_x >= prev_x && mouse_x < prev_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);
    bool next_hover = (mouse_x >= next_x && mouse_x < next_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);

    // Prev button
    vxair_fb_fill_rect(prev_x, btn_y, btn_w, btn_h, prev_hover ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH);
    vxair_fb_fill_rect(prev_x, btn_y, btn_w, 1, VxTheme::BORDER_BRIGHT);
    vxair_fb_fill_rect(prev_x, btn_y + btn_h - 1, btn_w, 1, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(prev_x, btn_y, 1, btn_h, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(prev_x + btn_w - 1, btn_y, 1, btn_h, VxTheme::BORDER_SUBTLE);
    const char* pl = "< Prev";
    for (int i = 0; pl[i]; i++) draw_abstract_char(prev_x + 18 + i * 10, btn_y + 9, pl[i], prev_hover ? accent : VxTheme::TEXT_SECONDARY);

    // Next button
    vxair_fb_fill_rect(next_x, btn_y, btn_w, btn_h, next_hover ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH);
    vxair_fb_fill_rect(next_x, btn_y, btn_w, 1, VxTheme::BORDER_BRIGHT);
    vxair_fb_fill_rect(next_x, btn_y + btn_h - 1, btn_w, 1, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(next_x, btn_y, 1, btn_h, VxTheme::BORDER_SUBTLE);
    vxair_fb_fill_rect(next_x + btn_w - 1, btn_y, 1, btn_h, VxTheme::BORDER_SUBTLE);
    const char* nl = "Next >";
    for (int i = 0; nl[i]; i++) draw_abstract_char(next_x + 18 + i * 10, btn_y + 9, nl[i], next_hover ? accent : VxTheme::TEXT_SECONDARY);

    if (clicked) {
        if (prev_hover) { current_image--; if (current_image < 0) current_image = 2; }
        if (next_hover) { current_image++; if (current_image > 2) current_image = 0; }
    }
}
