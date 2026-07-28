#pragma once
// V5 Calendar — VXUI-themed premium calendar with month grid
#include <stdint.h>

static void draw_app_calendar(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int ax = w.x + 20;
    int ay = w.y + 50;
    int aw = w.w - 40;
    int ah = w.h - 70;

    if (aw <= 0 || ah <= 0) return;

    // Month header bar
    VxPanel header{ax, ay, aw, 36, 0, VxTheme::SURFACE_HIGH};
    header.draw();
    const char* month = "JULY 2026";
    for (int i = 0; month[i]; i++)
        draw_abstract_char(ax + 16 + i * 10, ay + 12, month[i], VxTheme::TEXT_PRIMARY);
    // Accent underline
    vxair_fb_fill_rect(ax + 16, ay + 30, 80, 2, VxTheme::accent());

    // Day-of-week headers
    const char* dows[7] = {"S","M","T","W","T","F","S"};
    int grid_y = ay + 44;
    int cell_w = aw / 7;
    int cell_h = (ah - 44 - 16) / 5;
    if (cell_w < 8) cell_w = 8;
    if (cell_h < 8) cell_h = 8;

    for (int c = 0; c < 7; c++) {
        int cx = ax + c * cell_w;
        VxLabel dow{cx + cell_w / 2 - 4, grid_y, dows[c], VxTheme::TEXT_MUTED, VxTheme::FONT_SMALL};
        dow.draw();
    }

    // Grid lines
    vxair_fb_fill_rect(ax, grid_y + 16, aw, 1, VxTheme::BORDER_SUBTLE);

    // 5×7 day cells
    static int selected_day = -1;
    const int today = 19; // July 20 → index 19 (0-based from day 1)

    int grid_start_y = grid_y + 20;
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 7; c++) {
            int idx = r * 7 + c;
            int cx = ax + c * cell_w;
            int cy = grid_start_y + r * cell_h;
            int day_num = idx + 1; // Simple: 1-35

            bool is_today = (idx == today);
            bool is_sel = (idx == selected_day);
            bool hover = (mouse_x >= cx && mouse_x < cx + cell_w &&
                          mouse_y >= cy && mouse_y < cy + cell_h);

            // Cell background
            uint32_t bg = VxTheme::BASE_DARK;
            if (is_today) bg = VxTheme::accent_soft();
            else if (is_sel) bg = VxTheme::OVERLAY;
            else if (hover) bg = VxTheme::SURFACE_HIGH;

            vxair_fb_fill_rect(cx + 1, cy + 1, cell_w - 2, cell_h - 2, bg);
            if (is_today) {
                vxair_fb_fill_rect(cx + 1, cy + 1, cell_w - 2, 1, VxTheme::accent());
            }

            // Day number
            if (day_num <= 31) {
                char d[3];
                int dl = 0;
                if (day_num >= 10) { d[dl++] = '0' + day_num / 10; d[dl++] = '0' + day_num % 10; }
                else { d[dl++] = '0' + day_num; }
                d[dl] = 0;
                uint32_t col = is_today ? VxTheme::TEXT_PRIMARY : (hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);
                for (int i = 0; d[i]; i++)
                    draw_abstract_char(cx + 6 + i * 8, cy + 6, d[i], col);
            }

            // Click handling
            if (clicked && hover) {
                selected_day = idx;
            }
        }
    }

    // Grid vertical lines
    for (int c = 1; c < 7; c++) {
        vxair_fb_fill_rect(ax + c * cell_w, grid_y + 16, 1, grid_start_y + 5 * cell_h - grid_y - 16, VxTheme::BORDER_SUBTLE);
    }
    // Horizontal lines
    for (int r = 1; r < 5; r++) {
        vxair_fb_fill_rect(ax, grid_start_y + r * cell_h, aw, 1, VxTheme::BORDER_SUBTLE);
    }
}
