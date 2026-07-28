#ifndef APP_SETTINGS_HPP
#define APP_SETTINGS_HPP

static void draw_app_settings(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t accent = VxTheme::accent();

    // Sidebar and main area using VXUI panels
    int sidebar_w = 180;
    VxPanel sidebar = {w.x, w.y + VxTheme::TITLE_BAR_H, sidebar_w, w.h - VxTheme::TITLE_BAR_H, 0};
    sidebar.draw();
    VxPanel main_area = {w.x + sidebar_w, w.y + VxTheme::TITLE_BAR_H, w.w - sidebar_w, w.h - VxTheme::TITLE_BAR_H, 0};
    main_area.draw();

    // Draw Settings App Icon in sidebar top
    draw_app_icon(w.x + 20, w.y + 40, 4, false);
    VxLabel title_lbl = {w.x + 64, w.y + 50, "SETTINGS", accent, VxTheme::FONT_LARGE};
    title_lbl.draw();

    // Categories — VXUI buttons
    const char* cats[5] = {"INPUT", "APPEARANCE", "DESKTOP", "ACCESSIBILITY", "SYSTEM"};
    static int selected_cat = 0;

    for (int i = 0; i < 5; i++) {
        int cy = w.y + 100 + i * 40;
        bool active = (selected_cat == i);
        VxButton btn = {w.x + 10, cy, sidebar_w - 20, 30, cats[i], VX_BTN_DEFAULT, false, false, active};
        btn.check_hover(mouse_x, mouse_y);
        btn.draw();
        if (active) {
            vxr_fill_rect(w.x + 10, cy, 4, 30, accent);
        }
        if (clicked && btn.is_hovered) selected_cat = i;
    }

    int cx = w.x + sidebar_w + 30;
    int cy = w.y + 50;
    bool settings_changed = false;
    uint32_t text_color = VxTheme::TEXT_PRIMARY;
    uint32_t muted = VxTheme::TEXT_SECONDARY;

    auto draw_section_title = [&](const char* label, int y) {
        VxLabel lbl = {cx, y, label, text_color, VxTheme::FONT_LARGE};
        lbl.draw();
        vxr_fill_rect(cx, y + 14, w.w - sidebar_w - 60, 1, VxTheme::BORDER_SUBTLE);
    };

    auto draw_toggle = [&](int x, int y, const char* label, bool& value, int max_w) -> bool {
        VxLabel lbl = {x, y + 8, label, text_color, VxTheme::FONT_BODY};
        lbl.draw();
        int bw = 72, bh = 28;
        int bx = x + max_w - bw - 8;
        VxButton btn = {bx, y, bw, bh, value ? "ON" : "OFF", value ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
        btn.check_hover(mouse_x, mouse_y);
        btn.draw();
        if (clicked && btn.is_hovered) { value = !value; return true; }
        return false;
    };

    if (selected_cat == 0) {
        // INPUT
        draw_section_title("MOUSE & POINTER", cy);
        cy += 34;

        VxLabel lbl = {cx, cy, "SENSITIVITY", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        VxSlider sens_slider = {cx, cy + 14, w.w - sidebar_w - 60, 36, g_state.mouse_sensitivity_level};
        sens_slider.draw();
        
        // Draw percentage text inside the slider
        char pct_str[8];
        int pct = g_state.mouse_sensitivity_level;
        pct_str[0] = '0' + (pct / 100);
        pct_str[1] = '0' + ((pct % 100) / 10);
        pct_str[2] = '0' + (pct % 10);
        pct_str[3] = '%';
        pct_str[4] = 0;
        // Trim leading zeros
        char* pct_ptr = pct_str;
        if (pct_ptr[0] == '0' && pct < 100) pct_ptr++;
        if (pct_ptr[0] == '0' && pct < 10) pct_ptr++;
        
        VxLabel pct_lbl = {cx + w.w - sidebar_w - 90, cy + 26, pct_ptr, VxTheme::TEXT_PRIMARY, VxTheme::FONT_BODY};
        pct_lbl.draw();

        if (g_state.previous_left_down && sens_slider.handle_drag(mouse_x, mouse_y)) {
            g_state.mouse_sensitivity_level = sens_slider.value_pct;
            if (g_state.mouse_sensitivity_level < 1) g_state.mouse_sensitivity_level = 1;
            settings_changed = true;
        }
        cy += 70;
        if (draw_toggle(cx, cy, "LARGE CURSOR", g_state.large_cursor, w.w - sidebar_w - 60)) settings_changed = true;

    } else if (selected_cat == 1) {
        // APPEARANCE
        draw_section_title("ACCENT COLOR", cy);
        cy += 34;

        uint32_t colors[6] = {0xFF2D7FF9, 0xFF5AA0FF, 0xFF22C55E, 0xFFF59E0B, 0xFFEF4444, 0xFFA855F7};
        for (int i = 0; i < 6; i++) {
            int bx = cx + i * 45;
            bool active = (g_state.accent_color == colors[i]);
            VxButton swatch = {bx, cy, 36, 36, "", active ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
            swatch.check_hover(mouse_x, mouse_y);
            swatch.draw();
            vxr_fill_rect(bx + 4, cy + 4, 28, 28, colors[i]);
            if (clicked && swatch.is_hovered) {
                g_state.accent_color = colors[i];
                VxTheme::set_accent(colors[i]);
                settings_changed = true;
            }
        }
        cy += 54;

        draw_section_title("TASKBAR & CHROME", cy);
        cy += 34;

        VxLabel lbl = {cx, cy, "TASKBAR", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        VxButton btn1 = {cx, cy + 18, 90, 32, "COMPACT", g_state.compact_taskbar ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
        btn1.check_hover(mouse_x, mouse_y);
        btn1.draw();
        if (clicked && btn1.is_hovered) { g_state.compact_taskbar = true; settings_changed = true; }
        VxButton btn2 = {cx + 100, cy + 18, 90, 32, "NORMAL", !g_state.compact_taskbar ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
        btn2.check_hover(mouse_x, mouse_y);
        btn2.draw();
        if (clicked && btn2.is_hovered) { g_state.compact_taskbar = false; settings_changed = true; }
        cy += 64;

        if (draw_toggle(cx, cy, "SHOW TOP BAR", g_state.show_top_bar, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "DESKTOP GLOW", g_state.show_desktop_glow, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "WINDOW SHADOWS", g_state.show_window_shadows, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;

        VxLabel lbl2 = {cx, cy, "WALLPAPER", text_color, VxTheme::FONT_BODY};
        lbl2.draw();
        const char* wp_names[3] = {"GRADIENT", "DOTS", "NONE"};
        for (int i = 0; i < 3; i++) {
            VxButton btn = {cx + i * 78, cy + 20, 70, 28, wp_names[i], (g_state.wallpaper_mode == i) ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
            btn.check_hover(mouse_x, mouse_y);
            btn.draw();
            if (clicked && btn.is_hovered) { g_state.wallpaper_mode = i; settings_changed = true; }
        }

    } else if (selected_cat == 2) {
        // DESKTOP
        draw_section_title("CLOCK & WINDOWS", cy);
        cy += 34;

        if (draw_toggle(cx, cy, "SHOW SECONDS", g_state.show_seconds, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "24-HOUR CLOCK", g_state.hour_24, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "AUTO-CENTER WINDOWS", g_state.auto_center_windows, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "CLOSE CONFIRMATION", g_state.show_close_confirm, w.w - sidebar_w - 60)) settings_changed = true;

    } else if (selected_cat == 3) {
        // ACCESSIBILITY
        draw_section_title("ACCESSIBILITY", cy);
        cy += 34;

        if (draw_toggle(cx, cy, "FOCUS DIMMING", g_state.focus_dim, w.w - sidebar_w - 60)) settings_changed = true;
        cy += 42;
        if (draw_toggle(cx, cy, "HIGH CONTRAST", g_state.high_contrast, w.w - sidebar_w - 60)) settings_changed = true;

    } else if (selected_cat == 4) {
        // SYSTEM
        draw_section_title("STORAGE & ABOUT", cy);
        cy += 34;

        int used_blocks = 0;
        for (int i = 0; i < 10; i++) if (g_state.ram_files[i].in_use) used_blocks++;

        VxLabel cap_lbl = {cx, cy, "RAM FILES:", text_color, VxTheme::FONT_BODY};
        cap_lbl.draw();
        draw_abstract_char(cx + 120, cy, '0' + used_blocks, accent);
        draw_abstract_char(cx + 132, cy, '/', muted);
        draw_abstract_char(cx + 144, cy, '1', muted);
        draw_abstract_char(cx + 156, cy, '0', muted);
        cy += 40;

        VxLabel about_lbl = {cx, cy, "OS: VEXTRYN AIR V2", muted, VxTheme::FONT_BODY};
        about_lbl.draw();
        cy += 40;

        // Reset to defaults button
        VxButton reset_btn = {cx, cy, 160, 36, "RESET DEFAULTS", VX_BTN_ACTION, false, false};
        reset_btn.check_hover(mouse_x, mouse_y);
        reset_btn.draw();
        if (clicked && reset_btn.is_hovered) {
            g_state.mouse_sensitivity_level = 3;
            g_state.wallpaper_mode = 0;
            g_state.compact_taskbar = false;
            g_state.accent_color = 0xFF2D7FF9;
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
    }

    if (settings_changed) {
        uint8_t settings_buf[512] = {0};
        settings_buf[0] = 0xAA;
        settings_buf[1] = 0x55;
        settings_buf[2] = 0x01;
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
