#pragma once
#include <stdint.h>

// Assuming VxWindow is already defined before this header is included in the compositor.
struct VxWindow;

// Forward declare compositor drawing function
extern void vxair_fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

// Helper function to draw safely within a clipping region (the image area)
static void img_draw_rect_clipped(int x, int y, int w, int h, uint32_t color, int clip_x, int clip_y, int clip_w, int clip_h) {
    if (x < clip_x) {
        w -= (clip_x - x);
        x = clip_x;
    }
    if (y < clip_y) {
        h -= (clip_y - y);
        y = clip_y;
    }
    if (x + w > clip_x + clip_w) {
        w = clip_x + clip_w - x;
    }
    if (y + h > clip_y + clip_h) {
        h = clip_y + clip_h - y;
    }
    if (w > 0 && h > 0) {
        vxair_fb_fill_rect((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, color);
    }
}

void draw_app_image_viewer(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int ax = w.x + 2;
    int ay = w.y + 28;
    int aw = w.w - 4;
    int ah = w.h - 30;
    
    if (aw <= 0 || ah <= 0) return;
    
    // Dark Background for the whole app
    vxair_fb_fill_rect(ax, ay, aw, ah, 0x1E1E1E);

    static int current_image = 0;
    
    // Navigation buttons dimensions
    int btn_w = 60;
    int btn_h = 24;
    int prev_x = ax + aw / 2 - btn_w - 15;
    int next_x = ax + aw / 2 + 15;
    int btn_y = ay + ah - btn_h - 10;
    
    // Handle clicks
    if (clicked) {
        if (mouse_y >= btn_y && mouse_y <= btn_y + btn_h) {
            if (mouse_x >= prev_x && mouse_x <= prev_x + btn_w) {
                current_image--;
                if (current_image < 0) current_image = 2;
            } else if (mouse_x >= next_x && mouse_x <= next_x + btn_w) {
                current_image++;
                if (current_image > 2) current_image = 0;
            }
        }
    }
    
    // Draw Prev Button
    uint32_t prev_color = (mouse_x >= prev_x && mouse_x <= prev_x + btn_w && mouse_y >= btn_y && mouse_y <= btn_y + btn_h) ? 0x666666 : 0x444444;
    vxair_fb_fill_rect(prev_x, btn_y, btn_w, btn_h, prev_color);
    for (int i = 0; i < 8; ++i) {
        vxair_fb_fill_rect(prev_x + btn_w / 2 - i + 4, btn_y + btn_h / 2 - i, 2, i * 2, 0xFFFFFF);
    }
    
    // Draw Next Button
    uint32_t next_color = (mouse_x >= next_x && mouse_x <= next_x + btn_w && mouse_y >= btn_y && mouse_y <= btn_y + btn_h) ? 0x666666 : 0x444444;
    vxair_fb_fill_rect(next_x, btn_y, btn_w, btn_h, next_color);
    for (int i = 0; i < 8; ++i) {
        vxair_fb_fill_rect(next_x + btn_w / 2 + i - 4, btn_y + btn_h / 2 - i, 2, i * 2, 0xFFFFFF);
    }
    
    // Image Area dimensions
    int img_x = ax + 10;
    int img_y = ay + 10;
    int img_w = aw - 20;
    int img_h = ah - 40 - btn_h;
    
    if (img_w <= 0 || img_h <= 0) return;
    
    // Draw image contents based on current_image
    if (current_image == 0) {
        // Image 0: Sun over mountains
        img_draw_rect_clipped(img_x, img_y, img_w, img_h, 0x87CEEB, img_x, img_y, img_w, img_h); // Sky blue
        
        // Sun
        int sun_cx = img_x + img_w / 2;
        int sun_cy = img_y + img_h / 3;
        int sun_r = (img_w < img_h ? img_w : img_h) / 5;
        for (int dy = -sun_r; dy <= sun_r; ++dy) {
            for (int dx = -sun_r; dx <= sun_r; ++dx) {
                if (dx*dx + dy*dy <= sun_r*sun_r) {
                    img_draw_rect_clipped(sun_cx + dx, sun_cy + dy, 1, 1, 0xFFD700, img_x, img_y, img_w, img_h);
                }
            }
        }
        
        // Mountains
        int base_y = img_y + img_h;
        int m1_h = img_h / 2;
        for (int row = 0; row < m1_h; ++row) {
            img_draw_rect_clipped(img_x + img_w / 4 - row, base_y - m1_h + row, row * 2, 1, 0x808080, img_x, img_y, img_w, img_h);
        }
        int m2_h = img_h * 2 / 3;
        for (int row = 0; row < m2_h; ++row) {
            img_draw_rect_clipped(img_x + img_w * 3 / 4 - row, base_y - m2_h + row, row * 2, 1, 0x606060, img_x, img_y, img_w, img_h);
        }
    } else if (current_image == 1) {
        // Image 1: Abstract shapes
        img_draw_rect_clipped(img_x, img_y, img_w, img_h, 0x2E0854, img_x, img_y, img_w, img_h); // Deep purple
        
        // Circle
        int c_r = (img_w < img_h ? img_w : img_h) / 3;
        for (int dy = -c_r; dy <= c_r; ++dy) {
            for (int dx = -c_r; dx <= c_r; ++dx) {
                if (dx*dx + dy*dy <= c_r*c_r) {
                    img_draw_rect_clipped(img_x + img_w/3 + dx, img_y + img_h/3 + dy, 1, 1, 0xFF4500, img_x, img_y, img_w, img_h);
                }
            }
        }
        
        // Diamond
        for (int dy = -c_r; dy <= c_r; ++dy) {
            int width = c_r - (dy > 0 ? dy : -dy);
            img_draw_rect_clipped(img_x + img_w*2/3 - width, img_y + img_h*2/3 + dy, width*2, 1, 0x32CD32, img_x, img_y, img_w, img_h);
        }
    } else {
        // Image 2: House and Grass
        img_draw_rect_clipped(img_x, img_y, img_w, img_h, 0xADD8E6, img_x, img_y, img_w, img_h); // Sky
        img_draw_rect_clipped(img_x, img_y + img_h*3/4, img_w, img_h/4, 0x228B22, img_x, img_y, img_w, img_h); // Grass
        
        // House base
        int h_w = img_w / 3;
        int h_h = img_h / 3;
        int h_x = img_x + img_w / 3;
        int h_y = img_y + img_h*3/4 - h_h;
        img_draw_rect_clipped(h_x, h_y, h_w, h_h, 0xD2B48C, img_x, img_y, img_w, img_h); // Tan house
        
        // Roof
        for (int row = 0; row < h_h/2; ++row) {
            int roof_w = h_w / 2 + 10;
            int w = (row * roof_w) / (h_h/2);
            if (w > 0) {
                img_draw_rect_clipped(h_x + h_w/2 - w, h_y - h_h/2 + row, w*2, 1, 0xA52A2A, img_x, img_y, img_w, img_h); // Brown roof
            }
        }
        
        // Door
        img_draw_rect_clipped(h_x + h_w/2 - h_w/8, h_y + h_h - h_h/2, h_w/4, h_h/2, 0x8B4513, img_x, img_y, img_w, img_h);
    }
    
    // Draw a border around the image frame
    vxair_fb_fill_rect(img_x - 2, img_y - 2, img_w + 4, 2, 0x555555); // Top
    vxair_fb_fill_rect(img_x - 2, img_y + img_h, img_w + 4, 2, 0x555555); // Bottom
    vxair_fb_fill_rect(img_x - 2, img_y, 2, img_h, 0x555555); // Left
    vxair_fb_fill_rect(img_x + img_w, img_y, 2, img_h, 0x555555); // Right
}
