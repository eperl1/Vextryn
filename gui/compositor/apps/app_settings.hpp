#ifndef APP_SETTINGS_HPP
#define APP_SETTINGS_HPP

static void draw_app_settings(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t accent = VxTheme::accent();
    
    // Split into sidebar and main area using VXUI panels
    int sidebar_w = 180;
    VxPanel sidebar = {w.x, w.y + 28, sidebar_w, w.h - 28, 0};
    sidebar.draw();
    VxPanel main_area = {w.x + sidebar_w, w.y + 28, w.w - sidebar_w, w.h - 28, 0};
    main_area.draw();
    
    // Draw Settings App Icon in sidebar top
    draw_app_icon(w.x + 20, w.y + 40, 4, false);
    VxLabel title_lbl = {w.x + 64, w.y + 50, "SETTINGS", accent, VxTheme::FONT_LARGE};
    title_lbl.draw();

    // Categories — VXUI buttons
    const char* cats[4] = {"INPUT", "APPEARANCE", "THEME", "SYSTEM"};
    static int selected_cat = 0;
    
    for (int i=0; i<4; i++) {
        int cy = w.y + 100 + i*40;
        bool active = (selected_cat == i);
        VxButton btn = {w.x + 10, cy, sidebar_w - 20, 30, cats[i], VX_BTN_DEFAULT, false, false, active};
        btn.check_hover(mouse_x, mouse_y);
        btn.draw();
        if (active) {
            vxair_fb_fill_rect(w.x + 10, cy, 4, 30, accent);
        }
        if (clicked && btn.is_hovered) selected_cat = i;
    }

    int cx = w.x + sidebar_w + 30;
    int cy = w.y + 50;
    bool settings_changed = false;
    uint32_t text_color = VxTheme::TEXT_PRIMARY;
    uint32_t muted = VxTheme::TEXT_SECONDARY;

    if (selected_cat == 0) {
        // INPUT
        VxLabel lbl = {cx, cy, "MOUSE SENSITIVITY", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        
        for (int i=1; i<=5; i++) {
            int bx = cx + i*30 - 30;
            char lbl_digit[2] = {(char)('0'+i), 0};
            bool active = (g_state.mouse_sensitivity_level == i);
            VxButton btn = {bx, cy + 30, 28, 28, lbl_digit, active ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
            btn.check_hover(mouse_x, mouse_y);
            btn.draw();
            if (clicked && btn.is_hovered) { g_state.mouse_sensitivity_level = i; settings_changed = true; }
        }
    } else if (selected_cat == 1) {
        // APPEARANCE
        VxLabel lbl = {cx, cy, "TASKBAR STYLE", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        
        VxButton btn1 = {cx, cy + 30, 90, 32, "COMPACT", g_state.compact_taskbar ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
        btn1.check_hover(mouse_x, mouse_y);
        btn1.draw();
        if (clicked && btn1.is_hovered) { g_state.compact_taskbar = true; settings_changed = true; }
        
        VxButton btn2 = {cx + 100, cy + 30, 90, 32, "NORMAL", !g_state.compact_taskbar ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
        btn2.check_hover(mouse_x, mouse_y);
        btn2.draw();
        if (clicked && btn2.is_hovered) { g_state.compact_taskbar = false; settings_changed = true; }
    } else if (selected_cat == 2) {
        // THEME
        VxLabel lbl = {cx, cy, "ACCENT COLOR", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        
        uint32_t colors[5] = {VxTheme::ACCENT, 0xFF6B8E5A, 0xFFD4A04A, 0xFF9B6B9E, 0xFF5B7BA0};
        for (int i=0; i<5; i++) {
            int bx = cx + i*45;
            bool active = (g_state.accent_color == colors[i]);
            VxButton swatch = {bx, cy + 30, 36, 36, "", active ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false};
            swatch.check_hover(mouse_x, mouse_y);
            swatch.draw();
            vxair_fb_fill_rect(bx + 4, cy + 34, 28, 28, colors[i]);
            if (clicked && swatch.is_hovered) {
                g_state.accent_color = colors[i];
                VxTheme::set_accent(colors[i]);
                settings_changed = true;
            }
        }
    } else if (selected_cat == 3) {
        // SYSTEM
        VxLabel lbl = {cx, cy, "STORAGE: ATA BLOCK DEV", text_color, VxTheme::FONT_BODY};
        lbl.draw();
        
        int used_blocks = 0;
        for(int i=0; i<10; i++) if (g_state.ram_files[i].in_use) used_blocks++;
        
        VxLabel cap_lbl = {cx, cy + 30, "CAPACITY:", text_color, VxTheme::FONT_BODY};
        cap_lbl.draw();
        draw_abstract_char(cx + 120, cy + 30, '0' + used_blocks, accent);
        draw_abstract_char(cx + 132, cy + 30, '/', muted);
        draw_abstract_char(cx + 144, cy + 30, '1', muted);
        draw_abstract_char(cx + 156, cy + 30, '0', muted);
        
        VxLabel about_lbl = {cx, cy + 70, "OS: VEXTRYN AIR 1.0", muted, VxTheme::FONT_BODY};
        about_lbl.draw();
    }
    
    if (settings_changed) {
        // Let vxair_vxcomp.cpp handle the actual write logic by calling intercepted_settings_write
        // But since we can't easily call it without extern, we'll just write it correctly.
        uint8_t settings_buf[512] = {0};
        settings_buf[0] = 0xAA;
        settings_buf[1] = 0x55;
        settings_buf[2] = 0x01; 
        settings_buf[3] = g_state.mouse_sensitivity_level;
        settings_buf[4] = 0;
        settings_buf[5] = g_state.compact_taskbar;
        
        settings_buf[6] = (g_state.accent_color & 0xFF);
        settings_buf[7] = ((g_state.accent_color >> 8) & 0xFF);
        settings_buf[8] = ((g_state.accent_color >> 16) & 0xFF);
        settings_buf[9] = ((g_state.accent_color >> 24) & 0xFF);
        
        ata_write_sector(0, settings_buf);
    }
}
#endif
