#ifndef APP_SETTINGS_HPP
#define APP_SETTINGS_HPP

static void draw_app_settings(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t bg = 0xFF0F172A; // darker background
    uint32_t sidebar_bg = 0xFF1E293B; // sidebar
    uint32_t panel = 0xFF1E293B;
    uint32_t text_color = 0xFFF8FAFC;
    uint32_t highlight = 0xFF334155;
    uint32_t muted = 0xFF94A3B8;
    
    // Split into sidebar and main area
    int sidebar_w = 180;
    vxair_fb_fill_rect(w.x, w.y + 28, sidebar_w, w.h - 28, sidebar_bg);
    vxair_fb_fill_rect(w.x + sidebar_w, w.y + 28, w.w - sidebar_w, w.h - 28, bg);
    
    // Draw Settings App Icon in sidebar top
    draw_app_icon(w.x + 20, w.y + 40, 4, false);
    const char* title = "SETTINGS";
    for(int i=0; title[i]; i++) draw_abstract_char(w.x + 64 + i*10, w.y + 50, title[i], g_state.accent_color);

    // Categories
    const char* cats[4] = {"INPUT", "APPEARANCE", "THEME", "SYSTEM"};
    static int selected_cat = 0;
    
    for (int i=0; i<4; i++) {
        int cy = w.y + 100 + i*40;
        bool hover = (mouse_x >= w.x + 10 && mouse_x <= w.x + sidebar_w - 10 && mouse_y >= cy && mouse_y <= cy + 30);
        bool active = (selected_cat == i);
        
        if (hover || active) {
            vxair_fb_fill_rect(w.x + 10, cy, sidebar_w - 20, 30, active ? highlight : 0xFF273548);
        }
        if (active) {
            vxair_fb_fill_rect(w.x + 10, cy, 4, 30, g_state.accent_color);
        }
        
        for (int j=0; cats[i][j]; j++) {
            draw_abstract_char(w.x + 30 + j*10, cy + 10, cats[i][j], active ? text_color : muted);
        }
        
        if (clicked && hover) selected_cat = i;
    }

    int cx = w.x + sidebar_w + 30;
    int cy = w.y + 50;
    bool settings_changed = false;

    if (selected_cat == 0) {
        // INPUT
        const char* lbl_mouse = "MOUSE SENSITIVITY";
        for(int i=0; lbl_mouse[i]; i++) draw_abstract_char(cx + i*12, cy, lbl_mouse[i], text_color);
        
        for (int i=1; i<=5; i++) {
            int bx = cx + i*30 - 30;
            bool hover = (mouse_x >= bx && mouse_x <= bx + 24 && mouse_y >= cy + 30 && mouse_y <= cy + 54);
            bool active = (g_state.mouse_sensitivity_level == i);
            
            vxair_fb_fill_rect(bx, cy + 30, 24, 24, active ? g_state.accent_color : (hover ? highlight : panel));
            vxair_fb_fill_rect(bx+1, cy+31, 22, 22, active ? 0xFF0F172A : panel);
            draw_abstract_char(bx + 8, cy + 36, '0' + i, active ? g_state.accent_color : text_color);
            
            if (clicked && hover) { g_state.mouse_sensitivity_level = i; settings_changed = true; }
        }
    } else if (selected_cat == 1) {
        // APPEARANCE
        const char* lbl_task = "TASKBAR STYLE";
        for(int i=0; lbl_task[i]; i++) draw_abstract_char(cx + i*12, cy, lbl_task[i], text_color);
        
        int bx1 = cx;
        bool thover1 = (mouse_x >= bx1 && mouse_x <= bx1 + 80 && mouse_y >= cy + 30 && mouse_y <= cy + 60);
        vxair_fb_fill_rect(bx1, cy + 30, 80, 30, g_state.compact_taskbar ? g_state.accent_color : (thover1 ? highlight : panel));
        vxair_fb_fill_rect(bx1+1, cy + 31, 78, 28, g_state.compact_taskbar ? 0xFF0F172A : panel);
        const char* txt1 = "COMPACT";
        for(int i=0; txt1[i]; i++) draw_abstract_char(bx1 + 10 + i*8, cy + 40, txt1[i], g_state.compact_taskbar ? g_state.accent_color : text_color);
        if (clicked && thover1) { g_state.compact_taskbar = true; settings_changed = true; }
        
        int bx2 = cx + 100;
        bool thover2 = (mouse_x >= bx2 && mouse_x <= bx2 + 80 && mouse_y >= cy + 30 && mouse_y <= cy + 60);
        vxair_fb_fill_rect(bx2, cy + 30, 80, 30, !g_state.compact_taskbar ? g_state.accent_color : (thover2 ? highlight : panel));
        vxair_fb_fill_rect(bx2+1, cy + 31, 78, 28, !g_state.compact_taskbar ? 0xFF0F172A : panel);
        const char* txt2 = "NORMAL";
        for(int i=0; txt2[i]; i++) draw_abstract_char(bx2 + 15 + i*8, cy + 40, txt2[i], !g_state.compact_taskbar ? g_state.accent_color : text_color);
        if (clicked && thover2) { g_state.compact_taskbar = false; settings_changed = true; }
    } else if (selected_cat == 2) {
        // THEME
        const char* lbl_theme = "ACCENT COLOR";
        for(int i=0; lbl_theme[i]; i++) draw_abstract_char(cx + i*12, cy, lbl_theme[i], text_color);
        
        uint32_t colors[5] = {0xFF06B6D4, 0xFF0EA5E9, 0xFF3B82F6, 0xFF6366F1, 0xFF8B5CF6};
        for (int i=0; i<5; i++) {
            int bx = cx + i*45;
            bool hover = (mouse_x >= bx && mouse_x <= bx + 36 && mouse_y >= cy + 30 && mouse_y <= cy + 66);
            bool active = (g_state.accent_color == colors[i]);
            vxair_fb_fill_rect(bx, cy + 30, 36, 36, active ? text_color : (hover ? highlight : panel));
            vxair_fb_fill_rect(bx+2, cy + 32, 32, 32, colors[i]);
            if (clicked && hover) { g_state.accent_color = colors[i]; settings_changed = true; }
        }
    } else if (selected_cat == 3) {
        // SYSTEM
        const char* lbl_storage = "STORAGE: ATA BLOCK DEV";
        for(int i=0; lbl_storage[i]; i++) draw_abstract_char(cx + i*12, cy, lbl_storage[i], text_color);
        
        int used_blocks = 0;
        for(int i=0; i<10; i++) if (g_state.ram_files[i].in_use) used_blocks++;
        
        const char* val_storage1 = "CAPACITY:";
        for(int i=0; val_storage1[i]; i++) draw_abstract_char(cx + i*12, cy + 30, val_storage1[i], text_color);
        draw_abstract_char(cx + 120, cy + 30, '0' + used_blocks, g_state.accent_color);
        draw_abstract_char(cx + 132, cy + 30, '/', muted);
        draw_abstract_char(cx + 144, cy + 30, '1', muted);
        draw_abstract_char(cx + 156, cy + 30, '0', muted);
        
        const char* lbl_about = "OS: VEXTRYN AIR 1.0";
        for(int i=0; lbl_about[i]; i++) draw_abstract_char(cx + i*12, cy + 70, lbl_about[i], muted);
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
