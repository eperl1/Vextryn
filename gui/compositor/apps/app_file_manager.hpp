#pragma once

#include "../vxair_textinput.hpp"
#include "../../vxui/vxui_advanced.hpp"

// Shared text-input bindings for Files app
static VxTextInput file_name_input;
static VxTextInput file_content_input;
static int file_name_len = 0;
static uint64_t last_name_click_frame = 1000000;
static uint64_t last_content_click_frame = 1000000;

static void bind_name_input(RamFile& rf) {
    file_name_len = 0;
    while (rf.name[file_name_len] && file_name_len < 15) file_name_len++;
    file_name_input.buffer = rf.name;
    file_name_input.len = &file_name_len;
    file_name_input.max_len = 16;
}

static void bind_content_input(RamFile& rf) {
    file_content_input.buffer = rf.content;
    file_content_input.len = &rf.content_len;
    file_content_input.max_len = 512;
}

static void file_handle_key(char c) {
    if (g_state.file_selected_idx < 0 || g_state.file_selected_idx >= 10) return;
    RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
    if (!rf.in_use) return;

    if (g_state.file_rename_mode) {
        bind_name_input(rf);
        if (c == 26) { vx_copy(file_name_input); return; }
        if (c == 28) { vx_cut(file_name_input); save_files_to_disk(); return; }
        if (c == 29) { vx_paste(file_name_input); save_files_to_disk(); return; }
        if (c == '\n') {
            g_state.file_rename_mode = false;
            file_name_input.selection_anchor = file_name_input.caret_pos;
            save_files_to_disk();
        } else if (file_name_input.handle_key(c)) {
            save_files_to_disk();
        }
    } else if (g_state.file_preview_open) {
        bind_content_input(rf);
        if (c == 26) { vx_copy(file_content_input); return; }
        if (c == 28) { vx_cut(file_content_input); save_files_to_disk(); return; }
        if (c == 29) { vx_paste(file_content_input); save_files_to_disk(); return; }
        if (file_content_input.handle_key(c)) {
            save_files_to_disk();
        } else if (c == '\n') {
            if (file_content_input.insert_char('\n')) save_files_to_disk();
        }
    }
}

static const char* fm_sidebar_items[] = { "Home", "Documents", "Downloads", "Pictures" };
static int fm_sidebar_sel = 0;

static void draw_app_file_manager(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int top_y = w.y + VxTheme::TITLE_BAR_H;
    int content_h = w.h - VxTheme::TITLE_BAR_H;
    int sidebar_w = VxTheme::SETTINGS_SIDEBAR_W;

    // Background surface
    vxui_draw_rounded_rect(w.x, top_y, w.w, content_h, 0, VxTheme::SURFACE_1);

    // Left Sidebar Panel
    vxui_draw_rounded_rect(w.x, top_y, sidebar_w, content_h, 0, VxTheme::SURFACE_0);
    vxr_fill_rect(w.x + sidebar_w - 1, top_y, 1, content_h, VxTheme::BORDER_ALPHA);

    // Sidebar items
    for (int i = 0; i < 4; i++) {
        int item_y = top_y + 14 + i * 40;
        bool active = (fm_sidebar_sel == i);
        int item_x = w.x + 8;
        int item_w = sidebar_w - 16;
        int item_h = 34;

        bool hover = (mouse_x >= item_x && mouse_x <= item_x + item_w && mouse_y >= item_y && mouse_y <= item_y + item_h);

        if (active) {
            vxui_draw_rounded_rect(item_x, item_y, item_w, item_h, 6, VxTheme::ACCENT_SOFT);
            vxui_draw_rounded_rect(item_x, item_y + 6, 3, 22, 1, VxTheme::CYAN);
        } else if (hover) {
            vxui_draw_rounded_rect(item_x, item_y, item_w, item_h, 6, VxTheme::SURFACE_1);
        }

        // Folder Icon
        vxr_fill_rect(item_x + 12, item_y + 11, 12, 10, active ? VxTheme::accent() : VxTheme::TEXT_MUTED);
        vxr_fill_rect(item_x + 12, item_y + 9, 6, 3, active ? VxTheme::accent() : VxTheme::TEXT_MUTED);

        const char* name = fm_sidebar_items[i];
        for (int j = 0; name[j]; j++) {
            draw_abstract_char(item_x + 32 + j * 8, item_y + 11, name[j], active ? VxTheme::TEXT_PRIMARY : (hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY));
        }

        if (clicked && hover) {
            fm_sidebar_sel = i;
        }
    }

    int content_x = w.x + sidebar_w;
    int content_w = w.w - sidebar_w;

    // Content Toolbar Header (Breadcrumb & Actions)
    int tb_h = 44;
    vxui_draw_rounded_rect(content_x, top_y, content_w, tb_h, 0, VxTheme::SURFACE_0);
    vxr_fill_rect(content_x, top_y + tb_h - 1, content_w, 1, VxTheme::BORDER_ALPHA);

    // Breadcrumb path display
    const char* path_prefix = "Files > ";
    const char* cur_folder = fm_sidebar_items[fm_sidebar_sel];
    int px = content_x + 16;
    int py = top_y + 14;
    for (int i = 0; path_prefix[i]; i++) {
        draw_abstract_char(px + i * 8, py, path_prefix[i], VxTheme::TEXT_MUTED);
    }
    px += 8 * 8;
    for (int i = 0; cur_folder[i]; i++) {
        draw_abstract_char(px + i * 8, py, cur_folder[i], VxTheme::TEXT_PRIMARY);
    }

    // New File Button
    int btn_w = 96, btn_h = 28;
    int btn_x = content_x + content_w - btn_w - 14;
    int btn_y = top_y + (tb_h - btn_h) / 2;
    bool new_hover = (mouse_x >= btn_x && mouse_x <= btn_x + btn_w && mouse_y >= btn_y && mouse_y <= btn_y + btn_h);

    vxui_draw_rounded_rect(btn_x, btn_y, btn_w, btn_h, 6, new_hover ? VxTheme::accent_glow() : VxTheme::accent());
    draw_abstract_char(btn_x + 12, btn_y + 8, '+', 0xFFFFFFFF);
    const char* new_lbl = "New File";
    for (int i = 0; new_lbl[i]; i++) {
        draw_abstract_char(btn_x + 26 + i * 8, btn_y + 8, new_lbl[i], 0xFFFFFFFF);
    }

    if (clicked && new_hover) {
        for (int i = 0; i < 10; i++) {
            if (!g_state.ram_files[i].in_use) {
                g_state.ram_files[i].in_use = true;
                g_state.ram_files[i].name[0] = 'N'; g_state.ram_files[i].name[1] = 'E';
                g_state.ram_files[i].name[2] = 'W'; g_state.ram_files[i].name[3] = 0;
                g_state.ram_files[i].content_len = 0;
                g_state.file_selected_idx = i;
                g_state.file_preview_open = true;
                g_state.file_rename_mode = false;
                bind_content_input(g_state.ram_files[i]);
                file_content_input.caret_pos = 0;
                file_content_input.selection_anchor = 0;
                save_files_to_disk();
                break;
            }
        }
    }

    // Main View
    if (g_state.file_selected_idx >= 0 && g_state.file_selected_idx < 10 && g_state.ram_files[g_state.file_selected_idx].in_use && g_state.file_preview_open) {
        // Preview / Edit mode
        RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
        bind_name_input(rf);
        bind_content_input(rf);

        int view_top = top_y + tb_h + 12;
        int view_w = content_w - 32;
        int view_x = content_x + 16;

        // Document header card
        vxui_draw_rounded_rect(view_x, view_top, view_w, 40, 6, 0xFF182030);
        vxr_rounded_rect(view_x, view_top, view_w, 40, 6, 0xFF24324A);

        // Back button
        bool back_hover = (mouse_x >= view_x + 8 && mouse_x <= view_x + 58 && mouse_y >= view_top + 6 && mouse_y <= view_top + 34);
        vxui_draw_rounded_rect(view_x + 8, view_top + 6, 50, 28, 4, back_hover ? 0xFF28344D : 0xFF1E283B);
        const char* back_txt = "Back";
        for (int i = 0; back_txt[i]; i++) draw_abstract_char(view_x + 16 + i * 8, view_top + 14, back_txt[i], VxTheme::TEXT_PRIMARY);

        if (clicked && back_hover) {
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
        }

        // File name display / edit
        int nx = view_x + 70;
        int ny = view_top + 14;
        for (int i = 0; i < file_name_len; i++) {
            draw_abstract_char(nx + i * 8, ny, rf.name[i], VxTheme::TEXT_PRIMARY);
        }

        // Delete button
        bool del_hover = (mouse_x >= view_x + view_w - 68 && mouse_x <= view_x + view_w - 8 && mouse_y >= view_top + 6 && mouse_y <= view_top + 34);
        vxui_draw_rounded_rect(view_x + view_w - 68, view_top + 6, 60, 28, 4, del_hover ? VxTheme::DANGER : 0xFF2C191D);
        const char* del_txt = "Delete";
        for (int i = 0; del_txt[i]; i++) draw_abstract_char(view_x + view_w - 62 + i * 8, view_top + 14, del_txt[i], del_hover ? 0xFFFFFFFF : 0xFFF85149);

        if (clicked && del_hover) {
            rf.in_use = false;
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
            save_files_to_disk();
        }

        // Content Area Card
        int ed_top = view_top + 48;
        int ed_h = content_h - tb_h - 72;
        vxui_draw_rounded_rect(view_x, ed_top, view_w, ed_h, 6, 0xFF101622);
        vxr_rounded_rect(view_x, ed_top, view_w, ed_h, 6, 0xFF1C2638);

        int cx = view_x + 16;
        int cy = ed_top + 14;
        int line = 0, col = 0;
        int caret_cx = cx, caret_cy = cy;
        
        for (int i = 0; i < rf.content_len; i++) {
            if (i == file_content_input.caret_pos) { caret_cx = cx + col * 8; caret_cy = cy + line * 16; }
            if (rf.content[i] == '\n') { line++; col = 0; }
            else { draw_abstract_char(cx + col * 8, cy + line * 16, rf.content[i], VxTheme::TEXT_PRIMARY); col++; }
        }
        if (file_content_input.caret_pos == rf.content_len) { caret_cx = cx + col * 8; caret_cy = cy + line * 16; }
        
        if (w.focused && (frame % 60 < 30)) {
            vxr_fill_rect(caret_cx, caret_cy, 2, 12, VxTheme::accent());
        }

    } else {
        // List View / Grid Items
        int list_y = top_y + tb_h + 12;
        int item_h = 44;
        int drawn = 0;
        int item_w = content_w - 32;
        int item_x = content_x + 16;

        for (int i = 0; i < 10; i++) {
            if (!g_state.ram_files[i].in_use) continue;
            RamFile& rf = g_state.ram_files[i];
            
            int iy = list_y + drawn * (item_h + 6);
            if (iy + item_h > top_y + content_h) break;
            
            bool hover = (mouse_x >= item_x && mouse_x <= item_x + item_w && mouse_y >= iy && mouse_y <= iy + item_h);
            bool sel = (g_state.file_selected_idx == i);

            uint32_t bg_col = sel ? 0xFF1D283B : (hover ? 0xFF172030 : 0xFF141A26);
            vxui_draw_rounded_rect(item_x, iy, item_w, item_h, 6, bg_col);
            vxr_rounded_rect(item_x, iy, item_w, item_h, 6, sel ? 0xFF2A3A56 : 0xFF1E283A);

            // Document Icon
            vxr_fill_rect(item_x + 14, iy + 12, 12, 18, VxTheme::accent());
            vxr_fill_rect(item_x + 20, iy + 12, 6, 6, 0xFF141A26);

            for (int j = 0; j < 15 && rf.name[j]; j++) {
                draw_abstract_char(item_x + 36 + j * 8, iy + 14, rf.name[j], VxTheme::TEXT_PRIMARY);
            }

            char size_str[24] = "Size: 000 B";
            int sz = rf.content_len;
            size_str[6] = '0' + ((sz / 100) % 10);
            size_str[7] = '0' + ((sz / 10) % 10);
            size_str[8] = '0' + (sz % 10);
            for (int j = 0; size_str[j]; j++) {
                draw_abstract_char(item_x + item_w - 120 + j * 8, iy + 14, size_str[j], VxTheme::TEXT_MUTED);
            }

            if (clicked && hover) {
                g_state.file_selected_idx = i;
                g_state.file_preview_open = true;
            }
            drawn++;
        }

        // Empty state container if no files
        if (drawn == 0) {
            int empty_w = 280, empty_h = 100;
            int empty_x = content_x + (content_w - empty_w) / 2;
            int empty_y = top_y + tb_h + 40;

            vxui_draw_rounded_rect(empty_x, empty_y, empty_w, empty_h, 8, 0xFF141A26);
            vxr_rounded_rect(empty_x, empty_y, empty_w, empty_h, 8, 0xFF1E283A);

            const char* msg1 = "Folder is empty";
            const char* msg2 = "Click 'New File' to create";
            for (int i = 0; msg1[i]; i++) draw_abstract_char(empty_x + 72 + i * 8, empty_y + 30, msg1[i], VxTheme::TEXT_PRIMARY);
            for (int i = 0; msg2[i]; i++) draw_abstract_char(empty_x + 36 + i * 8, empty_y + 54, msg2[i], VxTheme::TEXT_MUTED);
        }
    }
}
