#pragma once

#include "../../vxui/vxui.hpp"

static void draw_studio_pill(int x, int y, int w, int h, const char* text, uint32_t col) {
    vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(x, y, w, h, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(x + 1, y + 1, w - 2, 1, VxColor::with_alpha(VxTheme::FG, 14));
    vxr_fill_rect(x + 10, y + h / 2 - 1, 16, 2, col);
    vx_text::draw(x + 32, y + 20, 12, text, VxTheme::TEXT_PRIMARY, VxTheme::GLASS_TINT);
}

static void draw_studio_button(int x, int y, int w, int h, const char* text, int mouse_x, int mouse_y, bool clicked, VxAppId target) {
    bool hover = (mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h);
    vxui_draw_rounded_rect(x, y, w, h, VxTheme::RADIUS_LG, hover ? VxTheme::SURFACE_3 : VxTheme::GLASS_TINT);
    vxr_rounded_border(x, y, w, h, VxTheme::RADIUS_LG, hover ? VxTheme::BORDER_STRONG_A : VxTheme::BORDER_ALPHA);
    vxr_fill_rect(x + 1, y + 1, w - 2, 1, VxColor::with_alpha(VxTheme::FG, 12));
    vx_text::draw(x + 18, y + 22, 13, text, hover ? VxTheme::FG : VxTheme::FG_SOFT, hover ? VxTheme::SURFACE_3 : VxTheme::GLASS_TINT);
    if (clicked && hover) open_app(target);
}

static void draw_app_studio(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame;
    uint32_t accent = VxTheme::accent();

    vxr_gradient_v(w.x, w.y + 28, w.w, w.h - 28, VxTheme::SURFACE_1, VxTheme::BASE_DEEP);
    vxr_fill_rect(w.x, w.y + 28, w.w, 60, VxTheme::ACCENT_SOFT);
    vxr_fill_rect(w.x, w.y + 88, w.w, 1, VxTheme::BORDER_ALPHA);

    const char* title = "Vextryn Studio";
    vx_text::draw(w.x + 18, w.y + 54, 16, title, VxTheme::TEXT_PRIMARY, VxTheme::SURFACE_1);
    const char* sub = "A visual compiler, framework, and build cockpit for the OS.";
    vx_text::draw(w.x + 18, w.y + 74, 13, sub, VxTheme::FG_SOFT, VxTheme::SURFACE_1);

    int left_x = w.x + 18;
    int left_y = w.y + 104;
    int left_w = (w.w - 54) * 58 / 100;
    int right_x = left_x + left_w + 16;
    int right_w = w.w - (right_x - w.x) - 18;

    vxui_draw_rounded_rect(left_x, left_y, left_w, 220, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(left_x, left_y, left_w, 220, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(left_x + 1, left_y + 1, left_w - 2, 1, VxColor::with_alpha(VxTheme::FG, 14));
    const char* build = "Build pipeline";
    vx_text::draw(left_x + 14, left_y + 22, 13, build, VxTheme::FG, VxTheme::GLASS_TINT);
    draw_studio_pill(left_x + 14, left_y + 38, left_w - 28, 28, "Compiler core online", accent);
    draw_studio_pill(left_x + 14, left_y + 72, left_w - 28, 28, "UI framework refreshed", VxTheme::CYAN);
    draw_studio_pill(left_x + 14, left_y + 106, left_w - 28, 28, "Desktop shell validated", accent);
    draw_studio_pill(left_x + 14, left_y + 140, left_w - 28, 28, "QEMU image ready", VxTheme::CYAN);

    vxr_fill_rect(left_x + 14, left_y + 186, left_w - 28, 10, VxTheme::SURFACE_2);
    vxr_fill_rect(left_x + 14, left_y + 186, (left_w - 28) * 78 / 100, 10, accent);

    vxui_draw_rounded_rect(right_x, left_y, right_w, 220, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(right_x, left_y, right_w, 220, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(right_x + 1, left_y + 1, right_w - 2, 1, VxColor::with_alpha(VxTheme::FG, 14));
    const char* tools = "Framework modules";
    vx_text::draw(right_x + 14, left_y + 22, 13, tools, VxTheme::FG, VxTheme::GLASS_TINT);

    const char* module_rows[4] = {
        "renderers and themes",
        "desktop windows and controls",
        "toolchain and build scripts",
        "native app runtime"
    };
    for (int i = 0; i < 4; i++) {
        vxui_draw_rounded_rect(right_x + 14, left_y + 40 + i * 40, right_w - 28, 30, 10, VxTheme::SURFACE_2);
        vxr_rounded_border(right_x + 14, left_y + 40 + i * 40, right_w - 28, 30, 10, VxTheme::BORDER_ALPHA);
        vxr_fill_rect(right_x + 15, left_y + 41 + i * 40, right_w - 30, 1, VxColor::with_alpha(VxTheme::FG, 12));
        vxr_fill_rect(right_x + 24, left_y + 52 + i * 40, 10, 2, i % 2 == 0 ? accent : VxTheme::CYAN);
        vx_text::draw(right_x + 42, left_y + 61 + i * 40, 12, module_rows[i], VxTheme::FG_SOFT, VxTheme::SURFACE_2);
    }

    int btn_y = left_y + 244;
    draw_studio_button(left_x, btn_y, 126, 30, "Open Terminal", mouse_x, mouse_y, clicked, VX_APP_TERMINAL);
    draw_studio_button(left_x + 136, btn_y, 116, 30, "Open Editor", mouse_x, mouse_y, clicked, VX_APP_CODE_EDITOR);
    draw_studio_button(left_x + 262, btn_y, 110, 30, "Open Store", mouse_x, mouse_y, clicked, VX_APP_STORE);
    draw_studio_button(left_x + 382, btn_y, 122, 30, "Open Dashboard", mouse_x, mouse_y, clicked, VX_APP_DASHBOARD);

    int log_y = btn_y + 48;
    vxui_draw_rounded_rect(left_x, log_y, w.w - 36, w.h - log_y - 18, VxTheme::RADIUS_LG, VxTheme::GLASS_TINT);
    vxr_rounded_border(left_x, log_y, w.w - 36, w.h - log_y - 18, VxTheme::RADIUS_LG, VxTheme::BORDER_ALPHA);
    vxr_fill_rect(left_x + 1, log_y + 1, w.w - 38, 1, VxColor::with_alpha(VxTheme::FG, 12));
    const char* log_title = "Compiler log";
    vx_text::draw(left_x + 14, log_y + 22, 13, log_title, VxTheme::FG, VxTheme::GLASS_TINT);
    const char* log_lines[3] = {
        "[ok] shell refreshed with the new layout",
        "[ok] new flagship apps registered in launcher",
        "[ok] qemu-visible boot path preserved"
    };
    for (int l = 0; l < 3; l++) {
        vx_text::draw(left_x + 14, log_y + 44 + l * 18, 12, log_lines[l], VxTheme::FG_SOFT, VxTheme::GLASS_TINT);
    }
}
