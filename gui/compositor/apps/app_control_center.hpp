#pragma once
// Vextryn Air Control Center — "Quick settings" popover, matching the Open Design
// vextryn-desktop-shell.html `.popover` spec: 300px wide, bottom-right above the
// taskbar, surface-1 body, switch rows (WiFi + Bluetooth), brightness/volume
// sliders with cyan fill.
#include <stdint.h>

struct VxCCLayout {
    VxRect panel;
    VxRect sw_wifi;
    VxRect sw_bt;
    VxRect slider_bright;
    VxRect slider_vol;
};

static VxCCLayout compute_cc_layout(uint32_t W, uint32_t H) {
    VxCCLayout L;
    int pw = 300;
    int ph = 244;
    int px = W - pw - 8;   // right: 8px
    int py = H - 64 - ph;  // bottom: 64px (above the 56px taskbar)
    L.panel = {px, py, pw, ph};

    // Switch rows (WiFi y=40, Bluetooth y=78) — 40x22 switches right-aligned
    L.sw_wifi = {px + pw - 12 - 40, py + 40, 40, 22};
    L.sw_bt   = {px + pw - 12 - 40, py + 78, 40, 22};

    // Slider rows (brightness y=130, volume y=176) — track areas
    L.slider_bright = {px + 12, py + 130, pw - 24, 44};
    L.slider_vol    = {px + 12, py + 176, pw - 24, 44};
    return L;
}

// Draw the Quick-settings popover. Returns true if a click was consumed inside.
static bool draw_control_center(uint32_t W, uint32_t H, int mx, int my, bool clicked) {
    VxCCLayout L = compute_cc_layout(W, H);
    uint32_t accent = VxTheme::accent();
    uint32_t cyan = VxTheme::CYAN;

    // Panel body — surface-1, 1px border-strong, 8px radius, floating shadow
    vxui_draw_dual_shadow(L.panel.x, L.panel.y, L.panel.w, L.panel.h, VxTheme::RADIUS_LG);
    vxui_draw_frosted_panel(L.panel.x, L.panel.y, L.panel.w, L.panel.h, VxTheme::RADIUS_LG, 6, VxColor::with_alpha(VxTheme::SURFACE_1, 186));
    vxr_rounded_border(L.panel.x, L.panel.y, L.panel.w, L.panel.h, VxTheme::RADIUS_LG, VxTheme::BORDER_STRONG_A);

    // Title
    vx_text::draw_bold(L.panel.x + 16, L.panel.y + 24, 13, "Quick settings", VxTheme::TEXT_PRIMARY, VxTheme::GLASS_TINT);

    // ---- Switch rows ----
    struct SwRow { const char* label; bool* state; };
    SwRow rows[2] = { {"WiFi", &g_state.wifi_enabled}, {"Bluetooth", &g_state.bluetooth_enabled} };
    VxRect sw_rects[2] = { L.sw_wifi, L.sw_bt };
    int row_y[2] = { L.panel.y + 38, L.panel.y + 76 };

    for (int r = 0; r < 2; r++) {
        // Row divider
        vxr_fill_rect(L.panel.x + 16, row_y[r] - 2, L.panel.w - 32, 1, VxTheme::BORDER_ALPHA);
        // Label
        vx_text::draw_bold(L.panel.x + 16, row_y[r] + 16, 13, rows[r].label, VxTheme::TEXT_PRIMARY, VxTheme::GLASS_TINT);

        bool on = *rows[r].state;
        const VxRect& sw = sw_rects[r];
        bool hover = sw.contains(mx, my);

        // Chip for WiFi
        if (r == 0) {
            const char* ssid = "vextryn-5G";
            int cw = 9 * 8 + 18; // text width + wifi glyph + padding
            vxui_draw_rounded_rect(sw.x - cw - 8, sw.y, cw, 22, 11, VxTheme::SURFACE_2);
            vxr_rounded_border(sw.x - cw - 8, sw.y, cw, 22, 11, VxTheme::BORDER_ALPHA);
            // wifi glyph (cyan arcs) at chip left
            int gx = sw.x - cw - 8 + 8, gy = sw.y + 5;
            vxr_fill_rect(gx, gy + 8, 6, 2, cyan);
            vxr_fill_rect(gx + 1, gy + 5, 4, 1, cyan);
            vxr_fill_rect(gx + 2, gy + 2, 2, 1, cyan);
            vx_text::draw(sw.x - cw - 8 + 18, sw.y + 16, 11, ssid, VxTheme::FG_SOFT, VxTheme::SURFACE_2);
        }

        // Switch track
        uint32_t track = on ? 0xFF134B4F : VxTheme::SURFACE_3;
        uint32_t track_border = on ? 0x8C00F0FF : VxTheme::BORDER_ALPHA;
        uint32_t knob_col = on ? cyan : VxTheme::MUTED;
        if (hover) track = on ? 0xFF15595E : VxTheme::SURFACE_2;
        vxui_draw_rounded_rect(sw.x, sw.y, sw.w, sw.h, 11, track);
        vxr_rounded_border(sw.x, sw.y, sw.w, sw.h, 11, track_border);
        int knob_x = on ? sw.x + sw.w - 18 : sw.x + 2;
        vxr_circle(knob_x + 8, sw.y + 11, 8, knob_col);

        if (clicked && hover) *rows[r].state = !(*rows[r].state);
    }

    // ---- Sliders ----
    const char* slider_labels[2] = { "Brightness", "Volume" };
    static int slider_val[2] = { 72, 61 };
    VxRect sl_rects[2] = { L.slider_bright, L.slider_vol };
    for (int s = 0; s < 2; s++) {
        const VxRect& sl = sl_rects[s];
        int y = sl.y;
        if (s == 1) vxr_fill_rect(L.panel.x + 16, y - 14, L.panel.w - 32, 1, VxTheme::BORDER_ALPHA);

        // Labels
        vx_text::draw_bold(sl.x, y + 12, 12, slider_labels[s], VxTheme::TEXT_SECONDARY, VxTheme::SURFACE_1);
        char pct[8] = {0};
        int v = slider_val[s];
        pct[0] = '0' + v / 10 % 10;
        pct[1] = '0' + v % 10;
        pct[2] = '%';
        int pct_w = vx_text::text_width(12, pct);
        vx_text::draw(sl.x + sl.w - pct_w, y + 12, 12, pct, VxTheme::TEXT_SECONDARY, VxTheme::SURFACE_1);

        // Track
        int track_x = sl.x;
        int track_y = y + 22;
        int track_w = sl.w;
        vxr_fill_rect(track_x, track_y, track_w, 4, 0x25D5DBE4);
        int fill_w = track_w * v / 100;
        vxr_fill_rect(track_x, track_y, fill_w, 4, cyan);
        // Thumb
        int knob_x = track_x + fill_w - 8;
        if (knob_x < track_x) knob_x = track_x;
        vxr_circle(knob_x + 8, track_y + 2, 8, VxTheme::FG);
        vxr_rounded_border(knob_x, track_y - 6, 16, 16, 8, 0xB300F0FF);

        // Click-to-set
        if (clicked && my >= track_y - 8 && my <= track_y + 12 && mx >= track_x && mx <= track_x + track_w) {
            slider_val[s] = (mx - track_x) * 100 / track_w;
            if (slider_val[s] < 0) slider_val[s] = 0;
            if (slider_val[s] > 100) slider_val[s] = 100;
        }
    }

    // Return whether the click was inside the panel
    return clicked && L.panel.contains(mx, my);
}
