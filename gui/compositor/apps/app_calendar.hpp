#pragma once

#include <stdint.h>

void draw_app_calendar(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int start_x = w.x + 2;
    int start_y = w.y + 28;
    int usable_w = w.w - 4;
    int usable_h = w.h - 30;

    if (usable_w <= 0 || usable_h <= 0) return;

    int cell_w = usable_w / 7;
    int cell_h = usable_h / 5;
    
    if (cell_w <= 2 || cell_h <= 2) return;

    static int selected_day = -1;
    // Today is July 20th according to system metadata, 
    // so we arbitrarily highlight the 20th grid cell (index 19)
    const int today_index = 19; 

    // Handle click
    if (clicked) {
        if (mouse_x >= start_x && mouse_x < start_x + cell_w * 7 &&
            mouse_y >= start_y && mouse_y < start_y + cell_h * 5) {
            int c = (mouse_x - start_x) / cell_w;
            int r = (mouse_y - start_y) / cell_h;
            if (c >= 0 && c < 7 && r >= 0 && r < 5) {
                selected_day = r * 7 + c;
            }
        }
    }

    // Draw background (acts as grid lines)
    vxair_fb_fill_rect(start_x, start_y, usable_w, usable_h, 0xFF888888);

    // Draw the 7x5 grid of days
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 7; ++c) {
            int index = r * 7 + c;
            int cx = start_x + c * cell_w;
            int cy = start_y + r * cell_h;

            uint32_t color = 0xFFFFFFFF; // White by default
            if (index == today_index) {
                color = 0xFFFF0000; // Red for today
            }
            if (index == selected_day) {
                color = 0xFF0000FF; // Blue for selected
            }

            vxair_fb_fill_rect(cx + 1, cy + 1, cell_w - 2, cell_h - 2, color);
        }
    }
}
