#ifndef VXUI_ADVANCED_HPP
#define VXUI_ADVANCED_HPP

#include "vxui.hpp"

// Advanced OS UI Controls

struct VxToggle {
    int x, y, w, h;
    bool is_on;
    bool is_hovered;

    void draw() {
        uint32_t bg = is_on ? VxTheme::accent() : VxTheme::SURFACE_HIGH;
        vxui_draw_rounded_rect(x, y, w, h, h/2, bg);
        
        int knob_r = h/2 - 2;
        int knob_x = is_on ? (x + w - knob_r*2 - 2) : (x + 2);
        vxr_circle(knob_x + knob_r, y + h/2, knob_r, 0xFFFFFFFF);
    }

    bool handle_click(int mx, int my) {
        if (VxRect{x, y, w, h}.contains(mx, my)) {
            is_on = !is_on;
            return true;
        }
        return false;
    }
};

struct VxSlider {
    int x, y, w, h;
    int value_pct; // 0 to 100

    void draw() {
        int track_h = 4;
        vxr_fill_rect(x, y + h/2 - track_h/2, w, track_h, VxTheme::SURFACE_HIGH);
        vxr_fill_rect(x, y + h/2 - track_h/2, (w * value_pct) / 100, track_h, VxTheme::accent());

        int knob_r = h/2;
        int knob_x = x + (w * value_pct) / 100 - knob_r;
        if (knob_x < x) knob_x = x;
        if (knob_x + knob_r*2 > x + w) knob_x = x + w - knob_r*2;
        
        vxr_circle(knob_x + knob_r, y + h/2, knob_r, 0xFFFFFFFF);
    }

    bool handle_drag(int mx, int my) {
        if (my >= y - 10 && my <= y + h + 10 && mx >= x && mx <= x + w) {
            value_pct = ((mx - x) * 100) / w;
            if (value_pct < 0) value_pct = 0;
            if (value_pct > 100) value_pct = 100;
            return true;
        }
        return false;
    }
};

struct VxProgressBar {
    int x, y, w, h;
    int progress_pct; // 0 to 100

    void draw() {
        vxui_draw_rounded_rect(x, y, w, h, h/2, VxTheme::SURFACE_HIGH);
        vxui_draw_rounded_rect(x, y, (w * progress_pct) / 100, h, h/2, VxTheme::accent());
    }
};

struct VxTabBar {
    int x, y, w, h;
    const char** tabs;
    int tab_count;
    int selected_index;

    void draw() {
        vxr_fill_rect(x, y, w, h, VxTheme::SURFACE);
        int tab_w = w / tab_count;
        for (int i = 0; i < tab_count; i++) {
            uint32_t text_col = (i == selected_index) ? VxTheme::accent() : VxTheme::TEXT_SECONDARY;
            if (i == selected_index) {
                vxr_fill_rect(x + i * tab_w, y + h - 2, tab_w, 2, VxTheme::accent());
            }
            
            int label_len = 0; for (; tabs[i][label_len]; label_len++);
            int tx = x + i * tab_w + (tab_w - label_len * 8) / 2;
            int ty = y + (h - 12) / 2;
            
            for (int j = 0; tabs[i][j]; j++) {
                draw_abstract_char(tx + j * 8, ty, tabs[i][j], text_col);
            }
        }
    }
};

struct VxScrollView {
    int x, y, w, h;
    int content_h;
    int scroll_y;
    
    void draw_scrollbar() {
        if (content_h <= h) return;
        int sb_w = 6;
        int sb_x = x + w - sb_w - 2;
        int sb_h = h * h / content_h;
        if (sb_h < 20) sb_h = 20;
        int sb_y = y + (scroll_y * (h - sb_h)) / (content_h - h);
        
        vxui_draw_rounded_rect(sb_x, sb_y, sb_w, sb_h, sb_w/2, VxTheme::SURFACE_HIGH);
    }
    
    void scroll(int amount) {
        scroll_y += amount;
        if (scroll_y < 0) scroll_y = 0;
        if (scroll_y > content_h - h) scroll_y = content_h - h;
        if (content_h <= h) scroll_y = 0;
    }
};

struct VxSidebar {
    int x, y, w, h;
    const char** items;
    int item_count;
    int selected_index;

    void draw() {
        vxr_fill_rect(x, y, w, h, VxTheme::BASE_DEEP);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_SUBTLE);
        
        int item_h = 40;
        for (int i = 0; i < item_count; i++) {
            int iy = y + 10 + i * item_h;
            if (i == selected_index) {
                vxui_draw_rounded_rect(x + 10, iy, w - 20, item_h - 4, 8, VxTheme::accent_soft());
                vxr_fill_rect(x + 10, iy + 8, 4, item_h - 20, VxTheme::accent());
            }
            uint32_t text_col = (i == selected_index) ? VxTheme::accent_glow() : VxTheme::TEXT_SECONDARY;
            for (int j = 0; items[i][j]; j++) {
                draw_abstract_char(x + 24, iy + (item_h - 12)/2, items[i][j], text_col);
                // quick hack to draw string by advancing x inline is hard without full font context, 
                // but this assumes 8px fixed width from simple char loop logic used in the file
            }
            // Real text drawing wrapper should ideally be used. 
        }
    }
    
    // Better string draw logic:
    void draw_item_text(int tx, int ty, const char* str, uint32_t col) {
        for (int j = 0; str[j]; j++) {
            draw_abstract_char(tx + j * 8, ty, str[j], col);
        }
    }

    void draw_better() {
        vxr_fill_rect(x, y, w, h, VxTheme::BASE_DEEP);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_SUBTLE);
        
        int item_h = 40;
        for (int i = 0; i < item_count; i++) {
            int iy = y + 10 + i * item_h;
            if (i == selected_index) {
                vxui_draw_rounded_rect(x + 10, iy, w - 20, item_h - 4, VxTheme::RADIUS_MD, VxTheme::accent_soft());
                vxui_draw_rounded_rect(x + 10, iy + 8, 4, item_h - 20, 2, VxTheme::accent());
            }
            uint32_t text_col = (i == selected_index) ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
            draw_item_text(x + 24, iy + (item_h - 12)/2, items[i], text_col);
        }
    }
};

struct VxListView {
    int x, y, w, h;
    const char** items;
    int item_count;
    int selected_index;
    int hover_index;
    int scroll_y;

    void draw() {
        vxr_fill_rect(x, y, w, h, VxTheme::SURFACE);
        
        int item_h = 48;
        int start_idx = scroll_y / item_h;
        int end_idx = start_idx + (h / item_h) + 1;
        if (end_idx > item_count) end_idx = item_count;
        
        for (int i = start_idx; i < end_idx; i++) {
            int iy = y + (i * item_h) - scroll_y;
            
            if (i == selected_index) {
                vxui_draw_rounded_rect(x + 8, iy + 4, w - 16, item_h - 8, VxTheme::RADIUS_MD, VxTheme::accent_dim());
            } else if (i == hover_index) {
                vxui_draw_rounded_rect(x + 8, iy + 4, w - 16, item_h - 8, VxTheme::RADIUS_MD, VxTheme::OVERLAY);
            }
            
            uint32_t text_col = (i == selected_index) ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
            for (int j = 0; items[i][j]; j++) {
                draw_abstract_char(x + 20 + j * 8, iy + (item_h - 12)/2, items[i][j], text_col);
            }
            vxr_fill_rect(x + 20, iy + item_h - 1, w - 40, 1, VxTheme::BORDER_SUBTLE);
        }
    }
};

struct VxModal {
    int x, y, w, h;
    const char* title;
    const char* message;
    
    void draw() {
        // Overlay shadow (requires compositor support for alpha or just pseudo-dim)
        // Draw the modal box centered
        vxui_draw_shadow(x, y, w, h, 8);
        vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_LG, VxTheme::SURFACE_HIGH);
        vxr_fill_rect(x, y, w, 1, VxTheme::BORDER_BRIGHT);
        vxr_fill_rect(x, y + h - 1, w, 1, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x, y, 1, h, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_STRONG);
        
        // Title
        for (int j = 0; title[j]; j++) {
            draw_abstract_char(x + 20 + j * 8, y + 20, title[j], VxTheme::TEXT_PRIMARY);
        }
        
        // Message
        for (int j = 0; message[j]; j++) {
            draw_abstract_char(x + 20 + j * 8, y + 50, message[j], VxTheme::TEXT_SECONDARY);
        }
    }
};

struct VxSegmentedControl {
    int x, y, w, h;
    const char** segments;
    int segment_count;
    int selected_index;
    
    void draw() {
        vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_MD, VxTheme::SURFACE);
        vxr_fill_rect(x, y, w, 1, VxTheme::BORDER_SUBTLE);
        
        int seg_w = w / segment_count;
        for (int i = 0; i < segment_count; i++) {
            int sx = x + i * seg_w;
            if (i == selected_index) {
                vxui_draw_rounded_rect(sx + 2, y + 2, seg_w - 4, h - 4, VxTheme::RADIUS_MD, VxTheme::SURFACE_HIGH);
                vxui_draw_shadow(sx + 2, y + 2, seg_w - 4, h - 4, 2);
            }
            
            uint32_t text_col = (i == selected_index) ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
            int label_len = 0; for (; segments[i][label_len]; label_len++);
            int tx = sx + (seg_w - label_len * 8) / 2;
            int ty = y + (h - 12) / 2;
            
            for (int j = 0; segments[i][j]; j++) {
                draw_abstract_char(tx + j * 8, ty, segments[i][j], text_col);
            }
        }
    }
};

#endif
