#pragma once

#include "../../vxui/vxui.hpp"

static void draw_dashboard_tile(int x, int y, int w, int h, const char* title, const char* value, uint32_t accent) {
    vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(x, y, w, h, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(x + 1, y + 1, w - 2, 1, VxColor::with_alpha(VxTheme::FG, 16));
    vxr_fill_rect(x + 14, y + 14, 28, 2, accent);
    vx_text::draw_bold(x + 16, y + 28, 12, title, VxTheme::MUTED, VxTheme::GLASS_TINT);
    vx_text::draw(x + 16, y + 58, 14, value, VxTheme::TEXT_PRIMARY, VxTheme::GLASS_TINT);
}

static void draw_dashboard_action(int x, int y, int w, int h, const char* label, int mouse_x, int mouse_y, bool clicked, VxAppId target) {
    bool hover = (mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h);
    vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_LG, hover ? VxTheme::SURFACE_3 : VxTheme::GLASS_TINT);
    vxr_rounded_border(x, y, w, h, VxTheme::RADIUS_LG, hover ? VxTheme::BORDER_STRONG_A : VxTheme::BORDER_ALPHA);
    vxr_fill_rect(x + 1, y + 1, w - 2, 1, VxColor::with_alpha(VxTheme::FG, 12));
    vx_text::draw(x + 18, y + 22, 13, label, hover ? VxTheme::FG : VxTheme::FG_SOFT, hover ? VxTheme::SURFACE_3 : VxTheme::GLASS_TINT);
    if (clicked && hover) {
        open_app(target);
    }
}

static void draw_app_dashboard(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame;
    uint32_t accent = VxTheme::accent();

    vxr_gradient_v(w.x, w.y + 28, w.w, w.h - 28, VxTheme::SURFACE_1, VxTheme::BASE_DEEP);
    vxr_fill_rect(w.x, w.y + 28, w.w, 72, VxTheme::ACCENT_SOFT);
    vxr_fill_rect(w.x, w.y + 98, w.w, 1, VxTheme::BORDER_ALPHA);

    const char* title = "Vextryn Air Dashboard";
    vx_text::draw(w.x + 20, w.y + 54, 16, title, VxTheme::TEXT_PRIMARY, VxTheme::SURFACE_1);
    const char* sub = "Launch tools, review system health, and jump into the studio.";
    vx_text::draw(w.x + 20, w.y + 74, 13, sub, VxTheme::FG_SOFT, VxTheme::SURFACE_1);

    int content_x = w.x + 20;
    int content_y = w.y + 114;
    int left_w = (w.w - 52) / 2;
    int right_x = content_x + left_w + 16;
    int tile_w = (left_w - 14) / 2;

    draw_dashboard_tile(content_x, content_y, tile_w, 84, "SYSTEM", "READY", accent);
    draw_dashboard_tile(content_x + tile_w + 14, content_y, tile_w, 84, "APPS", "21", VxTheme::CYAN);
    draw_dashboard_tile(content_x, content_y + 96, tile_w, 84, "BUILD", "GREEN", accent);
    draw_dashboard_tile(content_x + tile_w + 14, content_y + 96, tile_w, 84, "THEME", "NOVA", VxTheme::CYAN);

    vxui_draw_rounded_rect(right_x, content_y, left_w, 180, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(right_x, content_y, left_w, 180, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(right_x + 1, content_y + 1, left_w - 2, 1, VxColor::with_alpha(VxTheme::FG, 14));
    const char* section = "Pinned tools";
    vx_text::draw_bold(right_x + 14, content_y + 22, 13, section, VxTheme::FG, VxTheme::GLASS_TINT);

    int button_y = content_y + 38;
    draw_dashboard_action(right_x + 14, button_y + 0, left_w - 28, 30, "Open Files", mouse_x, mouse_y, clicked, VX_APP_FILES);
    draw_dashboard_action(right_x + 14, button_y + 38, left_w - 28, 30, "Open Terminal", mouse_x, mouse_y, clicked, VX_APP_TERMINAL);
    draw_dashboard_action(right_x + 14, button_y + 76, left_w - 28, 30, "Open Studio", mouse_x, mouse_y, clicked, VX_APP_STUDIO);
    draw_dashboard_action(right_x + 14, button_y + 114, left_w - 28, 30, "Open Settings", mouse_x, mouse_y, clicked, VX_APP_SETTINGS);

    int bottom_y = content_y + 194;
    vxui_draw_rounded_rect(content_x, bottom_y, w.w - 40, 110, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(content_x, bottom_y, w.w - 40, 110, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(content_x + 1, bottom_y + 1, w.w - 42, 1, VxColor::with_alpha(VxTheme::FG, 12));
    const char* log_title = "Recent activity";
    vx_text::draw_bold(content_x + 14, bottom_y + 22, 13, log_title, VxTheme::FG, VxTheme::GLASS_TINT);
    const char* lines[3] = {
        "Desktop environment upgraded.",
        "Launcher now shows dashboard + studio.",
        "Windows, dock, and shell styling refreshed."
    };
    for (int l = 0; l < 3; l++) {
        vx_text::draw(content_x + 14, bottom_y + 44 + l * 18, 12, lines[l], VxTheme::FG_SOFT, VxTheme::GLASS_TINT);
    }
}
