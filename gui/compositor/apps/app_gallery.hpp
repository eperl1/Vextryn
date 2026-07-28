#pragma once

#include "../../vxui/vxui_advanced.hpp"

static int gallery_selected_tab = 0;
static const char* gallery_tabs[] = { "Photos", "Albums", "Favorites", "Shared" };

static void draw_app_gallery(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    vxr_fill_rect(w.x, w.y + 28, w.w, w.h - 28, VxTheme::BASE_DEEP);

    // Tab bar at top
    VxTabBar tabs;
    tabs.x = w.x + (w.w - 400) / 2;
    tabs.y = w.y + 28 + 10;
    tabs.w = 400;
    tabs.h = 40;
    tabs.tabs = gallery_tabs;
    tabs.tab_count = 4;
    tabs.selected_index = gallery_selected_tab;
    
    if (clicked && mouse_x >= tabs.x && mouse_x < tabs.x + tabs.w && mouse_y >= tabs.y && mouse_y < tabs.y + tabs.h) {
        int clicked_tab = (mouse_x - tabs.x) / (tabs.w / tabs.tab_count);
        if (clicked_tab >= 0 && clicked_tab < tabs.tab_count) {
            gallery_selected_tab = clicked_tab;
        }
    }
    
    // Draw tab bar container
    vxui_draw_rounded_rect(tabs.x, tabs.y, tabs.w, tabs.h, VxTheme::RADIUS_MD, VxTheme::SURFACE);
    tabs.draw();

    // Image Grid
    int grid_y = tabs.y + tabs.h + 20;
    int cols = 4;
    int padding = 20;
    int img_w = (w.w - (cols + 1) * padding) / cols;
    if (img_w < 50) img_w = 50;
    int img_h = img_w * 3 / 4; // 4:3 aspect ratio

    int start_x = w.x + padding;
    
    for (int i = 0; i < 12; i++) {
        int row = i / cols;
        int col = i % cols;
        int ix = start_x + col * (img_w + padding);
        int iy = grid_y + row * (img_h + padding);
        
        if (iy + img_h > w.y + w.h) break;

        bool hover = (mouse_x >= ix && mouse_x < ix + img_w && mouse_y >= iy && mouse_y < iy + img_h);
        
        // Pseudo image - gradient/color mockup
        uint32_t color1 = VxColor::lerp(VxTheme::accent(), VxTheme::SUCCESS, i * 8, 100);
        uint32_t color2 = VxColor::lerp(VxTheme::DANGER, VxTheme::WARNING, i * 8, 100);
        
        if (hover) {
            vxui_draw_shadow(ix, iy, img_w, img_h, 8);
            iy -= 2; // lift effect
        }
        
        vxui_draw_rounded_rect(ix, iy, img_w, img_h, VxTheme::RADIUS_LG, color1);
        
        // Draw a simulated landscape inside
        vxr_fill_rect(ix, iy + img_h/2, img_w, img_h/2, color2);
        vxr_circle(ix + img_w/3, iy + img_h/3, img_w/6, 0xFFFDE047); // sun
        
        // Overlay for hover
        if (hover) {
            vxui_draw_rounded_rect(ix, iy, img_w, img_h, VxTheme::RADIUS_LG, 0x40FFFFFF);
        }
        
        // Inner border
        vxr_fill_rect(ix, iy, img_w, 1, 0x20FFFFFF);
        vxr_fill_rect(ix, iy, 1, img_h, 0x20FFFFFF);
    }
}
