#pragma once
// V5 Control Center — Premium quick-settings overlay panel
// This is the "wow" surface: a glassmorphic panel with toggle tiles,
// brightness/volume sliders, accent color picker, and quick actions.
#include <stdint.h>

struct VxCCLayout {
    VxRect panel;
    VxRect tiles[6];
    VxRect sliders[2];
    VxRect accent_swatch[6];
    VxRect close_btn;
};

static VxCCLayout compute_cc_layout(uint32_t W, uint32_t H) {
    VxCCLayout L;
    int pw = 340;
    int ph = 420;
    int px = W - pw - 16;
    int py = VxTheme::TOPBAR_H + 8;
    L.panel = {px, py, pw, ph};

    // 3×2 grid of toggle tiles
    int tile_w = (pw - 48) / 3;
    int tile_h = 72;
    int tile_gap = 12;
    int grid_x = px + 16;
    int grid_y = py + 56;
    for (int i = 0; i < 6; i++) {
        int r = i / 3;
        int c = i % 3;
        L.tiles[i] = {grid_x + c * (tile_w + tile_gap),
                       grid_y + r * (tile_h + tile_gap),
                       tile_w, tile_h};
    }

    // Sliders below tiles
    int slider_y = grid_y + 2 * (tile_h + tile_gap) + 12;
    int slider_w = pw - 32;
    L.sliders[0] = {px + 16, slider_y, slider_w, 40};
    L.sliders[1] = {px + 16, slider_y + 52, slider_w, 40};

    // Accent color swatches
    int swatch_y = slider_y + 116;
    int swatch_size = 28;
    int swatch_gap = 8;
    int swatch_total = 6 * (swatch_size + swatch_gap) - swatch_gap;
    int swatch_x = px + (pw - swatch_total) / 2;
    for (int i = 0; i < 6; i++) {
        L.accent_swatch[i] = {swatch_x + i * (swatch_size + swatch_gap), swatch_y, swatch_size, swatch_size};
    }

    // Close button
    L.close_btn = {px + pw - 36, py + 8, 24, 24};
    return L;
}

// Draw the Control Center overlay. Returns true if a click was consumed.
static bool draw_control_center(uint32_t W, uint32_t H, int mx, int my, bool clicked) {
    VxCCLayout L = compute_cc_layout(W, H);
    uint32_t accent = VxTheme::accent();

    // Backdrop dim
    vxair_fb_fill_rect(0, 0, W, H, 0x40000000);

    // Panel shadow
    vxui_draw_shadow(L.panel.x, L.panel.y, L.panel.w, L.panel.h, 20);

    // Panel body — frosted glass
    vxair_fb_fill_rect(L.panel.x, L.panel.y, L.panel.w, L.panel.h, VxTheme::GLASS_TINT);
    vxair_fb_fill_rect(L.panel.x, L.panel.y, L.panel.w, 1, VxTheme::BORDER_BRIGHT);
    vxair_fb_fill_rect(L.panel.x, L.panel.y + L.panel.h - 1, L.panel.w, 1, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(L.panel.x, L.panel.y, 1, L.panel.h, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(L.panel.x + L.panel.w - 1, L.panel.y, 1, L.panel.h, VxTheme::BORDER_STRONG);

    // Header
    const char* title = "Control Center";
    for (int i = 0; title[i]; i++)
        draw_abstract_char(L.panel.x + 16, L.panel.y + 16, title[i], VxTheme::TEXT_PRIMARY);
    vxair_fb_fill_rect(L.panel.x + 16, L.panel.y + 32, 100, 2, accent);

    // Close button
    bool close_hover = L.close_btn.contains(mx, my);
    vxair_fb_fill_rect(L.close_btn.x, L.close_btn.y, L.close_btn.w, L.close_btn.h,
                       close_hover ? VxTheme::DANGER : VxTheme::SURFACE_HIGH);
    for (int i = 0; i < 10; i++) {
        vxair_fb_fill_rect(L.close_btn.x + 7 + i, L.close_btn.y + 7 + i, 2, 2, VxTheme::TEXT_PRIMARY);
        vxair_fb_fill_rect(L.close_btn.x + 16 - i, L.close_btn.y + 7 + i, 2, 2, VxTheme::TEXT_PRIMARY);
    }

    // Toggle tiles
    struct TileInfo { const char* label; bool* state; uint32_t icon_color; };
    TileInfo tiles[6] = {
        {"WiFi",      &g_state.show_top_bar,      accent},
        {"Bluetooth", &g_state.show_desktop_glow, VxTheme::SUCCESS},
        {"AirDrop",   &g_state.show_window_shadows, VxTheme::WARNING},
        {"DND",       &g_state.focus_dim,          VxTheme::DANGER},
        {"Dark Mode", &g_state.high_contrast,      VxTheme::TEXT_PRIMARY},
        {"Large Ptr", &g_state.large_cursor,       VxTheme::TEXT_SECONDARY},
    };

    for (int i = 0; i < 6; i++) {
        const VxRect& t = L.tiles[i];
        bool hover = t.contains(mx, my);
        bool on = *tiles[i].state;

        // Tile background
        uint32_t bg = on ? VxTheme::accent_soft() : VxTheme::SURFACE_HIGH;
        if (hover) bg = on ? VxTheme::OVERLAY : VxTheme::SURFACE;
        vxair_fb_fill_rect(t.x, t.y, t.w, t.h, bg);
        // Border
        vxair_fb_fill_rect(t.x, t.y, t.w, 1, VxTheme::BORDER_BRIGHT);
        vxair_fb_fill_rect(t.x, t.y + t.h - 1, t.w, 1, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(t.x, t.y, 1, t.h, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(t.x + t.w - 1, t.y, 1, t.h, VxTheme::BORDER_SUBTLE);

        // Toggle indicator (top-right)
        uint32_t ind = on ? accent : VxTheme::BORDER_STRONG;
        vxair_fb_fill_rect(t.x + t.w - 16, t.y + 8, 8, 8, ind);
        if (on) {
            // Checkmark
            for (int j = 0; j < 4; j++)
                vxair_fb_fill_rect(t.x + t.w - 14 + j, t.y + 11 + j, 2, 2, VxTheme::TEXT_PRIMARY);
            for (int j = 0; j < 5; j++)
                vxair_fb_fill_rect(t.x + t.w - 11 + j, t.y + 13 - j, 2, 2, VxTheme::TEXT_PRIMARY);
        }

        // Label
        for (int j = 0; tiles[i].label[j]; j++)
            draw_abstract_char(t.x + 10 + j * 8, t.y + t.h - 18, tiles[i].label[j],
                               on ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);

        if (clicked && hover) {
            *tiles[i].state = !(*tiles[i].state);
        }
    }

    // Sliders — brightness and volume (visual only, click-to-set position)
    const char* slider_labels[2] = {"Brightness", "Volume"};
    for (int s = 0; s < 2; s++) {
        const VxRect& sl = L.sliders[s];
        // Background
        vxair_fb_fill_rect(sl.x, sl.y, sl.w, sl.h, VxTheme::SURFACE_HIGH);
        vxair_fb_fill_rect(sl.x, sl.y, sl.w, 1, VxTheme::BORDER_BRIGHT);
        vxair_fb_fill_rect(sl.x, sl.y + sl.h - 1, sl.w, 1, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(sl.x, sl.y, 1, sl.h, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(sl.x + sl.w - 1, sl.y, 1, sl.h, VxTheme::BORDER_SUBTLE);

        // Label
        for (int j = 0; slider_labels[s][j]; j++)
            draw_abstract_char(sl.x + 12, sl.y + 8, slider_labels[s][j], VxTheme::TEXT_SECONDARY);

        // Slider track
        int track_x = sl.x + 12;
        int track_y = sl.y + 26;
        int track_w = sl.w - 24;
        vxair_fb_fill_rect(track_x, track_y, track_w, 6, VxTheme::BASE_DARK);

        // Fill (static at ~70%)
        int fill_pct = 70;
        if (clicked && my >= track_y - 4 && my <= track_y + 10 && mx >= track_x && mx <= track_x + track_w) {
            fill_pct = (mx - track_x) * 100 / track_w;
        }
        int fill_w = track_w * fill_pct / 100;
        vxair_fb_fill_rect(track_x, track_y, fill_w, 6, accent);
        // Knob
        int knob_x = track_x + fill_w - 6;
        if (knob_x < track_x) knob_x = track_x;
        vxair_fb_fill_rect(knob_x, track_y - 4, 14, 14, VxTheme::TEXT_PRIMARY);
        vxair_fb_fill_rect(knob_x + 2, track_y - 2, 10, 10, accent);
    }

    // Accent color picker
    const char* ac_label = "Accent Color";
    int acy = L.accent_swatch[0].y - 16;
    for (int j = 0; ac_label[j]; j++)
        draw_abstract_char(L.panel.x + 16, acy, ac_label[j], VxTheme::TEXT_SECONDARY);

    uint32_t accent_options[6] = {
        0xFF3B8CFF, // Ice Blue (default)
        0xFF8B5CF6, // Purple
        0xFF10B981, // Emerald
        0xFFF59E0B, // Amber
        0xFFEF4444, // Red
        0xFFEC4899, // Pink
    };

    for (int i = 0; i < 6; i++) {
        const VxRect& sw = L.accent_swatch[i];
        bool sel = (VxTheme::accent() == accent_options[i]);
        bool hover = sw.contains(mx, my);

        // Selection ring
        if (sel || hover) {
            vxair_fb_fill_rect(sw.x - 3, sw.y - 3, sw.w + 6, sw.h + 6, sel ? accent : VxTheme::BORDER_BRIGHT);
        }
        // Swatch
        vxair_fb_fill_rect(sw.x, sw.y, sw.w, sw.h, accent_options[i]);

        if (clicked && hover) {
            g_state.accent_color = accent_options[i];
            VxTheme::set_accent(accent_options[i]);
        }
    }

    // Close button actually closes the CC
    if (clicked && close_hover) {
        g_state.control_center_open = false;
    }

    // Return whether the click was inside the panel
    return clicked && (L.panel.contains(mx, my) || close_hover);
}
