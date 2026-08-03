#pragma once

#include "../../vxui/vxui.hpp"

// Screenshot Capture Utility App (vxshot)
static bool g_shot_taken = false;
static int g_shot_flash_timer = 0;

static void draw_app_screenshot(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame; (void)clicked;
    vxr_fill_rect(w.x, w.y + 28, w.w, w.h - 28, VxTheme::SURFACE);

    int card_w = w.w - 40;
    int card_h = w.h - 80;
    int card_x = w.x + 20;
    int card_y = w.y + 50;

    vxr_rounded_rect(card_x, card_y, card_w, card_h, VxTheme::RADIUS_MD, VxTheme::BASE_DEEP);

    if (g_shot_taken) {
        const char* msg = "[ Screenshot Captured to /shots/screen01.png ]";
        for (int i = 0; msg[i]; i++) {
            draw_abstract_char(card_x + (card_w - 46 * 8) / 2 + i * 8, card_y + card_h / 2 - 10, msg[i], VxTheme::SUCCESS);
        }
    } else {
        const char* msg = "Capture full desktop screen or region selection";
        for (int i = 0; msg[i]; i++) {
            draw_abstract_char(card_x + (card_w - 47 * 8) / 2 + i * 8, card_y + card_h / 2 - 20, msg[i], VxTheme::TEXT_PRIMARY);
        }
    }

    VxButton cap_btn = { card_x + (card_w - 140) / 2, card_y + card_h - 50, 140, 36, "Take Capture", VX_BTN_PRIMARY, false, false, false, false };
    cap_btn.check_hover(mouse_x, mouse_y);
    if (cap_btn.handle_click(mouse_x, mouse_y)) {
        g_shot_taken = true;
    }
    cap_btn.draw();
}
