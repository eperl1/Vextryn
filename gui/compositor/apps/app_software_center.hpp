#pragma once

#include "../../vxui/vxui.hpp"

// Software / Package Center App (vxstore)
struct AppPackage {
    const char* name;
    const char* category;
    const char* version;
    bool installed;
};

static AppPackage g_store_packages[5] = {
    {"Vextryn Web Engine v2", "Network", "2.4.0", true},
    {"GCC Cross Toolchain", "Developer", "13.2.0", true},
    {"Python Interpreter", "Utilities", "3.11.4", false},
    {"DOOM Shareware Engine", "Games", "1.1.0", false},
    {"FFmpeg Media Codex", "Media", "6.0.0", false}
};

static void draw_app_software_center(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame; (void)clicked;
    // Top banner
    vxr_fill_rect(w.x, w.y + 28, w.w, 60, VxTheme::ACCENT_SOFT);
    vxr_fill_rect(w.x, w.y + 87, w.w, 1, VxTheme::ACCENT_GLOW);

    const char* title = "Vextryn Air Software Center";
    for (int i = 0; title[i]; i++) {
        draw_abstract_char(w.x + 20 + i * 8, w.y + 44, title[i], VxTheme::TEXT_PRIMARY);
    }

    const char* sub = "Explore, install, and update system application packages";
    for (int i = 0; sub[i]; i++) {
        draw_abstract_char(w.x + 20 + i * 8, w.y + 64, sub[i], VxTheme::ACCENT_GLOW);
    }

    // List area
    int list_y = w.y + 88;
    int list_h = w.h - 88;
    vxr_fill_rect(w.x, list_y, w.w, list_h, VxTheme::BASE_DEEP);

    for (int i = 0; i < 5; i++) {
        int card_y = list_y + 12 + i * 54;
        if (card_y + 50 > w.y + w.h) break;

        vxr_rounded_rect(w.x + 12, card_y, w.w - 24, 48, VxTheme::RADIUS_MD, VxTheme::SURFACE);
        vxr_fill_rect(w.x + 12, card_y, w.w - 24, 1, VxTheme::BORDER_BRIGHT);

        // Package Name
        for (int c = 0; g_store_packages[i].name[c]; c++) {
            draw_abstract_char(w.x + 24 + c * 8, card_y + 10, g_store_packages[i].name[c], VxTheme::TEXT_PRIMARY);
        }

        // Category & Version
        for (int c = 0; g_store_packages[i].category[c]; c++) {
            draw_abstract_char(w.x + 24 + c * 8, card_y + 28, g_store_packages[i].category[c], VxTheme::TEXT_MUTED);
        }

        // Install / Installed Button
        int btn_w = 90;
        int btn_x = w.x + w.w - 36 - btn_w;
        int btn_y = card_y + 8;

        if (g_store_packages[i].installed) {
            VxButton inst_btn = { btn_x, btn_y, btn_w, 32, "Installed", VX_BTN_SECONDARY, false, false, false, true };
            inst_btn.draw();
        } else {
            VxButton get_btn = { btn_x, btn_y, btn_w, 32, "Install", VX_BTN_PRIMARY, false, false, false, false };
            get_btn.check_hover(mouse_x, mouse_y);
            if (get_btn.handle_click(mouse_x, mouse_y)) {
                g_store_packages[i].installed = true;
            }
            get_btn.draw();
        }
    }
}
