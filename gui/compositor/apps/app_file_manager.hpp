#ifndef APP_FILE_MANAGER_HPP
#define APP_FILE_MANAGER_HPP

static void draw_app_file_manager(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t bg = 0xFF1E293B;
    uint32_t sidebar = 0xFF0F172A;
    uint32_t text_color = 0xFFF8FAFC;
    uint32_t accent = g_state.accent_color;
    uint32_t danger = 0xFFFF003C;
    uint32_t highlight = 0xFF334155;

    // Sidebar
    vxair_fb_fill_rect(w.x, w.y + 28, 120, w.h - 28, sidebar);
    
    // Disk label
    draw_abstract_char(w.x + 10, w.y + 40, 'D', accent);
    draw_abstract_char(w.x + 22, w.y + 40, 'A', accent);
    draw_abstract_char(w.x + 34, w.y + 40, 'T', accent);
    draw_abstract_char(w.x + 46, w.y + 40, 'A', accent);
    
    // New File Button (Wireframe)
    uint32_t new_btn_y = w.y + w.h - 40;
    bool new_hover = (mouse_x >= w.x + 10 && mouse_x <= w.x + 110 && mouse_y >= (int)new_btn_y && mouse_y <= (int)new_btn_y + 30);
    vxair_fb_fill_rect(w.x + 10, new_btn_y, 100, 30, new_hover ? text_color : accent);
    vxair_fb_fill_rect(w.x + 11, new_btn_y + 1, 98, 28, sidebar);
    draw_abstract_char(w.x + 45, new_btn_y + 10, '+', new_hover ? text_color : accent);
    if (clicked && new_hover) {
        for (int i=0; i<10; i++) {
            if (!g_state.ram_files[i].in_use) {
                g_state.ram_files[i].in_use = true;
                g_state.ram_files[i].name[0] = 'N';
                g_state.ram_files[i].name[1] = 'E';
                g_state.ram_files[i].name[2] = 'W';
                g_state.ram_files[i].name[3] = 0;
                g_state.ram_files[i].content_len = 0;
                g_state.file_selected_idx = i;
                g_state.file_preview_open = true;
                g_state.file_rename_mode = false;
                save_files_to_disk();
                break;
            }
        }
    }

    // Main area background
    vxair_fb_fill_rect(w.x + 120, w.y + 28, w.w - 120, w.h - 28, bg);
    // Vertical divider
    vxair_fb_fill_rect(w.x + 120, w.y + 28, 1, w.h - 28, 0xFF334155);

    if (g_state.file_selected_idx >= 0 && g_state.file_selected_idx < 10 && g_state.ram_files[g_state.file_selected_idx].in_use && g_state.file_preview_open) {
        // Preview / Edit mode
        RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
        
        // Toolbar
        vxair_fb_fill_rect(w.x + 121, w.y + 28, w.w - 121, 40, sidebar);
        vxair_fb_fill_rect(w.x + 121, w.y + 67, w.w - 121, 1, 0xFF334155);
        
        // Back button (wireframe)
        bool back_hover = (mouse_x >= w.x + 130 && mouse_x <= w.x + 170 && mouse_y >= w.y + 34 && mouse_y <= w.y + 60);
        vxair_fb_fill_rect(w.x + 130, w.y + 34, 40, 26, back_hover ? text_color : 0xFF334155);
        vxair_fb_fill_rect(w.x + 131, w.y + 35, 38, 24, sidebar);
        draw_abstract_char(w.x + 144, w.y + 40, '<', text_color);
        if (clicked && back_hover) {
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
        }

        // Rename Button
        bool rename_hover = (mouse_x >= w.x + 180 && mouse_x <= w.x + 240 && mouse_y >= w.y + 34 && mouse_y <= w.y + 60);
        vxair_fb_fill_rect(w.x + 180, w.y + 34, 60, 26, (rename_hover || g_state.file_rename_mode) ? accent : 0xFF334155);
        vxair_fb_fill_rect(w.x + 181, w.y + 35, 58, 24, sidebar);
        draw_abstract_char(w.x + 204, w.y + 40, 'R', (rename_hover || g_state.file_rename_mode) ? accent : text_color);
        if (clicked && rename_hover) {
            g_state.file_rename_mode = !g_state.file_rename_mode;
        }

        // Delete Button (wireframe with danger accent)
        bool del_hover = (mouse_x >= w.x + w.w - 70 && mouse_x <= w.x + w.w - 10 && mouse_y >= w.y + 34 && mouse_y <= w.y + 60);
        vxair_fb_fill_rect(w.x + w.w - 70, w.y + 34, 60, 26, del_hover ? danger : 0xFF334155);
        vxair_fb_fill_rect(w.x + w.w - 69, w.y + 35, 58, 24, sidebar);
        draw_abstract_char(w.x + w.w - 46, w.y + 40, 'D', del_hover ? danger : text_color);
        if (clicked && del_hover) {
            rf.in_use = false;
            g_state.file_preview_open = false;
            save_files_to_disk();
        }

        // Name
        int nx = w.x + 250;
        int ny = w.y + 40;
        for (int c=0; rf.name[c] != 0 && c<15; c++) {
            draw_abstract_char(nx, ny, rf.name[c], g_state.file_rename_mode ? accent : text_color);
            nx += 12;
        }
        if (g_state.file_rename_mode && (frame % 60 < 30)) {
            vxair_fb_fill_rect(nx, ny, 2, 14, accent);
        }

        // Content
        int cx = w.x + 135;
        int cy = w.y + 80;
        for (int i=0; i<rf.content_len; i++) {
            if (rf.content[i] == '\n') {
                cx = w.x + 135;
                cy += 20;
            } else {
                draw_abstract_char(cx, cy, rf.content[i], text_color);
                cx += 12;
                if (cx > w.x + w.w - 20) {
                    cx = w.x + 135;
                    cy += 20;
                }
            }
        }
        if (!g_state.file_rename_mode && (frame % 60 < 30)) {
            vxair_fb_fill_rect(cx, cy, 2, 14, text_color);
        }
        
    } else {
        // List mode
        int list_y = w.y + 40;
        bool any = false;
        for (int i=0; i<10; i++) {
            if (g_state.ram_files[i].in_use) {
                any = true;
                bool item_hover = (mouse_x >= w.x + 130 && mouse_x <= w.x + w.w - 20 && mouse_y >= list_y && mouse_y <= list_y + 36);
                
                // Subtle item highlight
                if (item_hover) vxair_fb_fill_rect(w.x + 130, list_y, w.w - 150, 36, 0xFF334155);
                
                // Icon (Wireframe document)
                vxair_fb_fill_rect(w.x + 140, list_y + 8, 16, 20, accent);
                vxair_fb_fill_rect(w.x + 141, list_y + 9, 14, 18, item_hover ? 0xFF334155 : bg);
                vxair_fb_fill_rect(w.x + 144, list_y + 12, 8, 1, accent);
                vxair_fb_fill_rect(w.x + 144, list_y + 16, 8, 1, accent);
                vxair_fb_fill_rect(w.x + 144, list_y + 20, 5, 1, accent);
                
                int nx = w.x + 170;
                for (int c=0; g_state.ram_files[i].name[c] != 0 && c<15; c++) {
                    draw_abstract_char(nx, list_y + 12, g_state.ram_files[i].name[c], text_color);
                    nx += 12;
                }
                
                if (clicked && item_hover) {
                    g_state.file_selected_idx = i;
                    g_state.file_preview_open = true;
                    g_state.file_rename_mode = false;
                }
                
                list_y += 44;
            }
        }
        if (!any) {
            draw_abstract_char(w.x + (w.w - 120)/2 + 80, w.y + w.h/2, 'E', 0xFF64748B);
            draw_abstract_char(w.x + (w.w - 120)/2 + 92, w.y + w.h/2, 'M', 0xFF64748B);
            draw_abstract_char(w.x + (w.w - 120)/2 + 104, w.y + w.h/2, 'P', 0xFF64748B);
            draw_abstract_char(w.x + (w.w - 120)/2 + 116, w.y + w.h/2, 'T', 0xFF64748B);
            draw_abstract_char(w.x + (w.w - 120)/2 + 128, w.y + w.h/2, 'Y', 0xFF64748B);
        }
    }
}
#endif

