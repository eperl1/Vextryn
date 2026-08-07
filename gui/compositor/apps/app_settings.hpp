#ifndef APP_SETTINGS_HPP
#define APP_SETTINGS_HPP

static void draw_settings_text(int x, int y, const char* str, uint32_t color) {
    for (int i = 0; str[i]; i++) {
        draw_abstract_char(x + i * 9, y, str[i], color);
    }
}

// Small abstract nav glyph (16px cell) — design: lucide-style line icons
static void draw_settings_nav_icon(int x, int y, int kind, uint32_t color) {
    if (kind == 0) { // gear
        vxr_circle(x + 8, y + 8, 4, color);
        vxr_circle(x + 8, y + 8, 7, color);
        for (int i = 0; i < 8; i++) {
            int dx = (i == 0 ? 0 : (i == 4 ? 0 : (i < 4 ? 1 : -1)));
            int dy = (i == 2 ? 0 : (i == 6 ? 0 : (i < 2 || i == 7 ? -1 : 1)));
            vxr_fill_rect(x + 8 + dx * 8, y + 8 + dy * 8, 2, 2, color);
        }
    } else if (kind == 1) { // palette
        vxui_draw_rounded_rect(x, y + 2, 16, 12, 4, color);
        vxr_fill_rect(x + 3, y + 6, 3, 3, VxTheme::SURFACE_0);
        vxr_fill_rect(x + 10, y + 6, 3, 3, VxTheme::SURFACE_0);
    } else if (kind == 2) { // mouse
        vxui_draw_rounded_rect(x + 4, y, 8, 14, 4, color);
        vxr_fill_rect(x + 7, y + 2, 2, 5, VxTheme::SURFACE_0);
    } else if (kind == 3) { // clock
        vxr_circle(x + 8, y + 8, 7, color);
        vxr_fill_rect(x + 8, y + 4, 2, 5, color);
        vxr_fill_rect(x + 8, y + 9, 5, 2, color);
    } else { // accessibility (person)
        vxr_circle(x + 8, y + 2, 3, color);
        vxui_draw_rounded_rect(x + 4, y + 8, 8, 8, 4, color);
    }
}

// Design 40x22 toggle switch (off: surface-3, on: cyan-tinted + cyan knob)
static void draw_settings_switch(int x, int y, bool on) {
    uint32_t track_col = on ? 0xFF0D4B4F : VxTheme::SURFACE_3;
    vxui_draw_rounded_rect(x, y, 40, 22, 11, track_col);
    vxr_rounded_border(x, y, 40, 22, 11, on ? 0x8C00F0FF : VxTheme::BORDER_ALPHA);
    int knob_x = on ? (x + 40 - 18) : (x + 2);
    vxui_draw_rounded_rect(knob_x, y + 2, 16, 18, 9, on ? VxTheme::CYAN : VxTheme::MUTED);
}

// Design selector button: surface-2 bg, border-strong, text + chevron
static void draw_settings_select(int x, int y, const char* text, uint32_t accent) {
    int w = 0;
    for (int i = 0; text[i]; i++) w += 9;
    w += 24;
    vxui_draw_rounded_rect(x, y, w, 30, 8, VxTheme::SURFACE_2);
    vxr_rounded_border(x, y, w, 30, 8, VxTheme::BORDER_STRONG_A);
    draw_settings_text(x + 9, y + 10, text, VxTheme::FG_SOFT);
    vxr_fill_rect(x + w - 14, y + 11, 2, 2, VxTheme::MUTED);
    vxr_fill_rect(x + w - 12, y + 13, 2, 2, VxTheme::MUTED);
    vxr_fill_rect(x + w - 10, y + 15, 2, 2, VxTheme::MUTED);
}

static void draw_app_settings(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t accent = VxTheme::accent();
    bool settings_changed = false;

    // Window surface area bounds (inside title bar)
    int sidebar_w = VxTheme::SETTINGS_SIDEBAR_W;
    int content_w = w.w - sidebar_w;
    int content_h = w.h - VxTheme::TITLE_BAR_H;
    int top_y = w.y + VxTheme::TITLE_BAR_H;

    // Left Sidebar panel (surface-0, 1px border right)
    vxui_draw_rounded_rect(w.x, top_y, sidebar_w, content_h, 0, VxTheme::SURFACE_0);
    vxr_fill_rect(w.x + sidebar_w - 1, top_y, 1, content_h, VxTheme::BORDER_ALPHA);

    // Right Main Area surface (surface-1)
    vxui_draw_rounded_rect(w.x + sidebar_w, top_y, content_w, content_h, 0, VxTheme::SURFACE_1);

    // Sidebar App Branding Header
    draw_app_icon(w.x + 14, top_y + 14, 22, false);
    draw_settings_text(w.x + 46, top_y + 15, "Settings", VxTheme::FG_STRONG);
    draw_settings_text(w.x + 46, top_y + 29, "Vextryn Air OS", VxTheme::MUTED);

    vxr_fill_rect(w.x + 12, top_y + 52, sidebar_w - 24, 1, VxTheme::BORDER_ALPHA);

    // Sidebar search field (design .set-search: 32px, surface-2, cyan focus)
    int s_x = w.x + 12;
    int s_y = top_y + 62;
    int s_w = sidebar_w - 24;
    vxui_draw_rounded_rect(s_x, s_y, s_w, 32, 8, VxTheme::SURFACE_2);
    vxr_rounded_border(s_x, s_y, s_w, 32, 8, VxTheme::BORDER_ALPHA);
    vxr_circle(s_x + 16, s_y + 16, 5, VxTheme::MUTED);
    vxr_fill_rect(s_x + 20, s_y + 20, 5, 2, VxTheme::MUTED);
    const char* s_txt = "Search settings";
    for (int i = 0; s_txt[i]; i++) draw_abstract_char(s_x + 28 + i * 7, s_y + 12, s_txt[i], VxTheme::MUTED);

    // Section header
    draw_settings_text(w.x + 14, top_y + 108, "SYSTEM", VxTheme::MUTED);

    // Categories (icon + label + sub, active = 13% accent container)
    const char* cats[5] = {"SYSTEM & STORAGE", "APPEARANCE & SHELL", "MOUSE & INPUT", "CLOCK & LAYOUT", "ACCESSIBILITY"};
    const char* cat_sub[5] = {"Storage & Info", "Theme & Shell", "Sensitivity", "Taskbar & Time", "Display & Focus"};
    static int selected_cat = 0;

    for (int i = 0; i < 5; i++) {
        int cy = top_y + 124 + i * 36;
        int item_x = w.x + 8;
        int item_w = sidebar_w - 16;
        int item_h = 34;

        bool hover = (mouse_x >= item_x && mouse_x <= item_x + item_w && mouse_y >= cy && mouse_y <= cy + item_h);

        uint32_t bg_col = 0;
        if (selected_cat == i) bg_col = VxTheme::ACCENT_SOFT;
        else if (hover) bg_col = VxTheme::SURFACE_1;
        if (bg_col != 0) {
            vxui_draw_rounded_rect(item_x, cy, item_w, item_h, 8, bg_col);
        }
        if (selected_cat == i) {
            vxui_draw_rounded_rect(item_x, cy + 5, 3, 24, 2, VxTheme::CYAN);
        }

        draw_settings_nav_icon(item_x + 14, cy + 9, i, (selected_cat == i) ? accent : VxTheme::MUTED);
        draw_settings_text(item_x + 38, cy + 5, cats[i], (selected_cat == i) ? VxTheme::FG_STRONG : (hover ? VxTheme::FG : VxTheme::FG_SOFT));
        draw_settings_text(item_x + 38, cy + 19, cat_sub[i], VxTheme::MUTED);

        if (clicked && hover) {
            selected_cat = i;
        }
    }

    // Sidebar footer (storage + version + kbd hints)
    int foot_y = top_y + content_h - 104;
    vxr_fill_rect(w.x + 12, foot_y, sidebar_w - 24, 1, VxTheme::BORDER_ALPHA);
    draw_settings_text(w.x + 14, foot_y + 12, "STORAGE", VxTheme::MUTED);
    draw_settings_text(w.x + 14, foot_y + 26, "12.4 GB used", VxTheme::FG_SOFT);
    int fb_w = sidebar_w - 28;
    int fb_x = w.x + 14;
    vxui_draw_rounded_rect(fb_x, foot_y + 40, fb_w, 4, 2, VxTheme::SURFACE_3);
    vxui_draw_rounded_rect(fb_x, foot_y + 40, fb_w * 62 / 100, 4, 2, VxTheme::CYAN);
    draw_settings_text(w.x + 14, foot_y + 52, "Vextryn Air v1.0", VxTheme::MUTED);
    draw_settings_text(w.x + 14, foot_y + 68, "Alt+T Terminal  Alt+Tab Switch", VxTheme::MUTED);

    // ===== Right Content Pane =====
    int cx = w.x + sidebar_w + 24;
    int cy = top_y + 18;
    int card_w = content_w - 48;

    const char* header_titles[5] = {
        "System & Storage",
        "Appearance & Shell",
        "Mouse & Input",
        "Clock & Desktop",
        "Accessibility"
    };

    const char* header_descs[5] = {
        "Manage system parameters, RAM storage, and OS defaults.",
        "Customize dark shell tones, accent color, and shadows.",
        "Adjust mouse sensitivity levels and cursor pointer size.",
        "Configure taskbar clock format and window auto-centering.",
        "Enhance visual accessibility, contrast, and window focus dimming."
    };

    // Sticky pane-head
    draw_settings_text(cx, cy, header_titles[selected_cat], VxTheme::FG_STRONG);
    draw_settings_text(cx, cy + 14, header_descs[selected_cat], VxTheme::MUTED);
    vxr_fill_rect(cx, cy + 34, card_w, 1, VxTheme::BORDER_ALPHA);

    cy += 50;

    // Group card helper: header + rows
    auto draw_group_head = [&](int x, int y, int w_size, const char* title) {
        vxui_draw_rounded_rect(x, y, w_size, 30, 8, 0xFF202020);
        vxr_rounded_border(x, y, w_size, 30, 8, VxTheme::BORDER_ALPHA);
        vxr_fill_rect(x, y + 30, w_size, 1, VxTheme::BORDER_ALPHA);
        draw_settings_text(x + 14, y + 10, title, VxTheme::MUTED);
    };

    // Design toggle row (inside group card): label + desc left, switch right
    auto draw_toggle_card = [&](int x, int y, int w_size, const char* title, const char* desc, bool& val) -> bool {
        int card_h = 42;
        bool card_hover = (mouse_x >= x && mouse_x <= x + w_size && mouse_y >= y && mouse_y <= y + card_h);

        if (card_hover) {
            vxui_draw_rounded_rect(x, y, w_size, card_h, 0, VxTheme::SURFACE_1);
        }
        vxr_fill_rect(x, y + card_h - 1, w_size, 1, VxTheme::BORDER_ALPHA);

        draw_settings_text(x + 14, y + 6, title, VxTheme::FG);
        draw_settings_text(x + 14, y + 21, desc, VxTheme::MUTED);

        int sw_x = x + w_size - 54;
        int sw_y = y + (card_h - 22) / 2;
        draw_settings_switch(sw_x, sw_y, val);

        if (clicked && card_hover) {
            val = !val;
            return true;
        }
        return false;
    };

    if (selected_cat == 0) {
        // SYSTEM & STORAGE
        draw_group_head(cx, cy, card_w, "SYSTEM");
        cy += 30;
        if (draw_toggle_card(cx, cy, card_w, "Auto-center windows", "Center new window instances on initial launch", g_state.auto_center_windows)) settings_changed = true;
        cy += 42;
        if (draw_toggle_card(cx, cy, card_w, "Close confirmation", "Require confirmation dialog before closing windows", g_state.show_close_confirm)) settings_changed = true;
        cy += 42;

        // RAM Storage row
        int card_h = 42;
        vxr_fill_rect(cx, cy + card_h - 1, card_w, 1, VxTheme::BORDER_ALPHA);
        draw_settings_text(cx + 14, cy + 6, "RAM storage", VxTheme::FG);
        draw_settings_text(cx + 14, cy + 21, "In-memory file blocks", VxTheme::MUTED);

        int used_blocks = 0;
        for (int i = 0; i < 10; i++) if (g_state.ram_files[i].in_use) used_blocks++;

        char ram_str[32] = "Files: 0 / 10 Blocks";
        ram_str[7] = '0' + (used_blocks % 10);
        int vb_w = 0;
        for (int i = 0; ram_str[i]; i++) vb_w += 9;
        draw_settings_text(cx + card_w - vb_w - 14, cy + 6, ram_str, VxTheme::FG_SOFT);

        int bar_w = card_w - 150;
        int bar_x = cx + 14;
        int bar_y = cy + 27;
        vxui_draw_rounded_rect(bar_x, bar_y, bar_w, 4, 2, VxTheme::SURFACE_3);
        int fill_w = (bar_w * used_blocks) / 10;
        if (fill_w > 0) {
            vxui_draw_rounded_rect(bar_x, bar_y, fill_w, 4, 2, VxTheme::CYAN);
        }
        cy += 50;

        // Reset Defaults action button
        int btn_w = 160, btn_h = 32;
        bool rst_hover = (mouse_x >= cx && mouse_x <= cx + btn_w && mouse_y >= cy && mouse_y <= cy + btn_h);
        vxui_draw_rounded_rect(cx, cy, btn_w, btn_h, 8, rst_hover ? VxTheme::SURFACE_2 : VxTheme::SURFACE_0);
        vxr_rounded_border(cx, cy, btn_w, btn_h, 8, VxTheme::BORDER_STRONG_A);
        draw_settings_text(cx + 18, cy + 10, "Reset defaults", rst_hover ? VxTheme::FG : VxTheme::FG_SOFT);

        if (clicked && rst_hover) {
            g_state.mouse_sensitivity_level = 50;
            g_state.wallpaper_mode = 0;
            g_state.compact_taskbar = false;
            g_state.accent_color = 0xFF1677FF;
            VxTheme::set_accent(g_state.accent_color);
            g_state.show_top_bar = true;
            g_state.show_desktop_glow = true;
            g_state.show_window_shadows = true;
            g_state.focus_dim = false;
            g_state.high_contrast = false;
            g_state.large_cursor = false;
            g_state.show_seconds = false;
            g_state.hour_24 = true;
            g_state.auto_center_windows = false;
            g_state.show_close_confirm = false;
            settings_changed = true;
        }

    } else if (selected_cat == 1) {
        // APPEARANCE & SHELL
        draw_group_head(cx, cy, card_w, "APPEARANCE");
        cy += 30;

        // Accent Color row
        vxr_fill_rect(cx, cy + 41, card_w, 1, VxTheme::BORDER_ALPHA);
        draw_settings_text(cx + 14, cy + 6, "Accent color", VxTheme::FG);
        draw_settings_text(cx + 14, cy + 21, "System highlight accent", VxTheme::MUTED);

        uint32_t colors[6] = {0xFF1677FF, 0xFF38BDF8, 0xFF3FB950, 0xFFF59E0B, 0xFFF85149, 0xFFA855F7};
        for (int i = 0; i < 6; i++) {
            int sw_x = cx + card_w - 6 * 28 - 14 + i * 28;
            int sw_y = cy + 10;
            bool sw_active = (g_state.accent_color == colors[i]);
            bool sw_hover = (mouse_x >= sw_x && mouse_x <= sw_x + 22 && mouse_y >= sw_y && mouse_y <= sw_y + 22);

            vxui_draw_rounded_rect(sw_x, sw_y, 22, 22, 11, colors[i]);
            if (sw_active) {
                vxr_rounded_rect(sw_x - 2, sw_y - 2, 26, 26, 13, VxTheme::CYAN);
            } else if (sw_hover) {
                vxr_rounded_border(sw_x - 1, sw_y - 1, 24, 24, 12, VxTheme::BORDER_STRONG_A);
            }

            if (clicked && sw_hover) {
                g_state.accent_color = colors[i];
                VxTheme::set_accent(colors[i]);
                settings_changed = true;
            }
        }
        cy += 50;

        if (draw_toggle_card(cx, cy, card_w, "Desktop glow", "Enable background illumination gradient", g_state.show_desktop_glow)) settings_changed = true;
        cy += 42;
        if (draw_toggle_card(cx, cy, card_w, "Window shadows", "Render soft elevation drop shadows under windows", g_state.show_window_shadows)) settings_changed = true;

    } else if (selected_cat == 2) {
        // MOUSE & INPUT
        draw_group_head(cx, cy, card_w, "MOUSE & INPUT");
        cy += 30;

        // Pointer sensitivity slider row
        vxr_fill_rect(cx, cy + 48, card_w, 1, VxTheme::BORDER_ALPHA);
        draw_settings_text(cx + 14, cy + 6, "Pointer sensitivity", VxTheme::FG);
        draw_settings_text(cx + 14, cy + 21, "Mouse movement speed", VxTheme::MUTED);

        int sl_w = card_w - 150;
        int sl_x = cx + 14;
        int sl_y = cy + 34;

        int level = g_state.mouse_sensitivity_level;
        if (level < 1) level = 1;
        if (level > 100) level = 100;

        vxui_draw_rounded_rect(sl_x, sl_y, sl_w, 4, 2, VxTheme::SURFACE_3);
        int fill_w = (sl_w * level) / 100;
        if (fill_w > 0) {
            vxui_draw_rounded_rect(sl_x, sl_y, fill_w, 4, 2, VxTheme::CYAN);
        }
        int handle_x = sl_x + fill_w - 7;
        if (handle_x < sl_x) handle_x = sl_x;
        if (handle_x > sl_x + sl_w - 15) handle_x = sl_x + sl_w - 15;
        vxui_draw_rounded_rect(handle_x, sl_y - 5, 15, 15, 8, VxTheme::FG);
        vxr_rounded_border(handle_x, sl_y - 5, 15, 15, 8, 0x8C00F0FF);

        char pct_str[8];
        pct_str[0] = '0' + (level / 100);
        pct_str[1] = '0' + ((level % 100) / 10);
        pct_str[2] = '0' + (level % 10);
        pct_str[3] = '%';
        pct_str[4] = 0;
        char* pct_ptr = pct_str;
        if (pct_ptr[0] == '0' && level < 100) pct_ptr++;
        if (pct_ptr[0] == '0' && level < 10) pct_ptr++;

        draw_settings_text(cx + card_w - 40, cy + 30, pct_ptr, VxTheme::FG_SOFT);

        if (g_state.previous_left_down && mouse_x >= sl_x && mouse_x <= sl_x + sl_w && mouse_y >= sl_y - 6 && mouse_y <= sl_y + 18) {
            int new_val = ((mouse_x - sl_x) * 100) / sl_w;
            if (new_val < 1) new_val = 1;
            if (new_val > 100) new_val = 100;
            if (new_val != g_state.mouse_sensitivity_level) {
                g_state.mouse_sensitivity_level = new_val;
                settings_changed = true;
            }
        }

        cy += 60;

        if (draw_toggle_card(cx, cy, card_w, "Large cursor", "Increase mouse cursor size for high visibility", g_state.large_cursor)) settings_changed = true;

    } else if (selected_cat == 3) {
        // CLOCK & DESKTOP
        draw_group_head(cx, cy, card_w, "CLOCK & DESKTOP");
        cy += 30;
        if (draw_toggle_card(cx, cy, card_w, "Show seconds", "Display seconds on system tray clock", g_state.show_seconds)) settings_changed = true;
        cy += 42;
        if (draw_toggle_card(cx, cy, card_w, "24-hour clock", "Use 24-hour time format (HH:MM) instead of 12-hour", g_state.hour_24)) settings_changed = true;

    } else if (selected_cat == 4) {
        // ACCESSIBILITY
        draw_group_head(cx, cy, card_w, "ACCESSIBILITY");
        cy += 30;
        if (draw_toggle_card(cx, cy, card_w, "Focus dimming", "Darken inactive background windows for focus", g_state.focus_dim)) settings_changed = true;
        cy += 42;
        if (draw_toggle_card(cx, cy, card_w, "High contrast", "Enable high contrast borders and sharp UI highlights", g_state.high_contrast)) settings_changed = true;
    }

    // Save changes if modified
    if (settings_changed) {
        uint8_t settings_buf[512] = {0};
        settings_buf[0] = 0xAA;
        settings_buf[1] = 0x55;
        settings_buf[2] = 0x02; // Updated to v2 format
        settings_buf[3] = g_state.mouse_sensitivity_level;
        settings_buf[4] = g_state.wallpaper_mode;
        settings_buf[5] = g_state.compact_taskbar;
        write_u32_le(&settings_buf[6], g_state.accent_color);
        settings_buf[10] = g_state.show_top_bar;
        settings_buf[11] = g_state.show_desktop_glow;
        settings_buf[12] = g_state.show_window_shadows;
        settings_buf[13] = g_state.focus_dim;
        settings_buf[14] = g_state.high_contrast;
        settings_buf[15] = g_state.large_cursor;
        settings_buf[16] = g_state.show_seconds;
        settings_buf[17] = g_state.hour_24;
        settings_buf[18] = g_state.auto_center_windows;
        settings_buf[19] = g_state.show_close_confirm;
        ata_write_sector(0, settings_buf);
    }
}
#endif
