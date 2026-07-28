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
    vxr_fill_rect(w.x, w.y + 28, w.w, w.h - 28, VxTheme::BASE_DEEP);

    // Sidebar
    VxSidebar sidebar;
    sidebar.x = w.x;
    sidebar.y = w.y + 28;
    sidebar.w = 140;
    sidebar.h = w.h - 28;
    sidebar.items = fm_sidebar_items;
    sidebar.item_count = 4;
    sidebar.selected_index = fm_sidebar_sel;
    
    if (clicked && mouse_x >= sidebar.x && mouse_x < sidebar.x + sidebar.w && mouse_y >= sidebar.y) {
        int clicked_idx = (mouse_y - sidebar.y - 10) / 40;
        if (clicked_idx >= 0 && clicked_idx < sidebar.item_count) fm_sidebar_sel = clicked_idx;
    }
    sidebar.draw_better();

    // New File Button
    VxButton new_btn = { w.x + 10, w.y + w.h - 50, 120, 36, "New File", VX_BTN_PRIMARY, false, false, false, false };
    new_btn.check_hover(mouse_x, mouse_y);
    if (new_btn.handle_click(mouse_x, mouse_y)) {
        for (int i=0; i<10; i++) {
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
    new_btn.draw();

    int content_x = sidebar.x + sidebar.w;
    int content_w = w.w - sidebar.w;

    if (g_state.file_selected_idx >= 0 && g_state.file_selected_idx < 10 && g_state.ram_files[g_state.file_selected_idx].in_use && g_state.file_preview_open) {
        // Preview / Edit mode
        RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
        bind_name_input(rf);
        bind_content_input(rf);

        vxr_fill_rect(content_x, w.y + 28, content_w, 50, VxTheme::SURFACE);
        vxr_fill_rect(content_x, w.y + 77, content_w, 1, VxTheme::BORDER_SUBTLE);

        VxButton back_btn = { content_x + 10, w.y + 35, 60, 32, "Back", VX_BTN_SECONDARY, false, false, false, false };
        back_btn.check_hover(mouse_x, mouse_y);
        if (back_btn.handle_click(mouse_x, mouse_y)) {
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
        }
        back_btn.draw();

        VxButton ren_btn = { content_x + 80, w.y + 35, 80, 32, "Rename", g_state.file_rename_mode ? VX_BTN_PRIMARY : VX_BTN_SECONDARY, false, false, false, false };
        ren_btn.check_hover(mouse_x, mouse_y);
        if (ren_btn.handle_click(mouse_x, mouse_y)) {
            g_state.file_rename_mode = !g_state.file_rename_mode;
            if (g_state.file_rename_mode) { bind_name_input(rf); file_name_input.select_all(); }
            else { file_name_input.selection_anchor = file_name_input.caret_pos; }
        }
        ren_btn.draw();

        VxButton del_btn = { content_x + content_w - 90, w.y + 35, 80, 32, "Delete", VX_BTN_DEFAULT, false, false, false, false };
        del_btn.check_hover(mouse_x, mouse_y);
        if (del_btn.handle_click(mouse_x, mouse_y)) {
            rf.in_use = false;
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
            save_files_to_disk();
        }
        del_btn.draw();

        // Name field
        int nx = content_x + 170;
        int ny = w.y + 45;
        for (int i = 0; i < file_name_len; i++) {
            draw_abstract_char(nx + i * 8, ny, rf.name[i], VxTheme::TEXT_PRIMARY);
        }
        if (g_state.file_rename_mode && w.focused && (frame % 60 < 30)) {
            vxr_fill_rect(nx + file_name_input.caret_pos * 8, ny, 2, 12, VxTheme::accent());
        }

        // Content field
        int cx = content_x + 20;
        int cy = w.y + 90;
        int max_lines = (w.h - 100) / 16;
        int line = 0, col = 0;
        int caret_cx = cx, caret_cy = cy;
        
        for (int i = 0; i < rf.content_len; i++) {
            if (i == file_content_input.caret_pos) { caret_cx = cx + col * 8; caret_cy = cy + line * 16; }
            if (rf.content[i] == '\n') { line++; col = 0; }
            else { draw_abstract_char(cx + col * 8, cy + line * 16, rf.content[i], VxTheme::TEXT_PRIMARY); col++; }
        }
        if (file_content_input.caret_pos == rf.content_len) { caret_cx = cx + col * 8; caret_cy = cy + line * 16; }
        
        if (!g_state.file_rename_mode && w.focused && (frame % 60 < 30)) {
            vxr_fill_rect(caret_cx, caret_cy, 2, 12, VxTheme::accent());
        }
        
    } else {
        // List View
        int list_y = w.y + 28 + 20;
        int item_h = 48;
        int drawn = 0;
        for (int i = 0; i < 10; i++) {
            if (!g_state.ram_files[i].in_use) continue;
            RamFile& rf = g_state.ram_files[i];
            
            int iy = list_y + drawn * item_h;
            if (iy + item_h > w.y + w.h) break;
            
            bool hover = (mouse_x >= content_x + 20 && mouse_x < content_x + content_w - 20 && mouse_y >= iy && mouse_y < iy + item_h);
            if (g_state.file_selected_idx == i) {
                vxui_draw_rounded_rect(content_x + 20, iy, content_w - 40, item_h, VxTheme::RADIUS_MD, VxTheme::accent_dim());
            } else if (hover) {
                vxui_draw_rounded_rect(content_x + 20, iy, content_w - 40, item_h, VxTheme::RADIUS_MD, VxTheme::OVERLAY);
            }
            
            if (clicked && hover) {
                g_state.file_selected_idx = i;
                g_state.file_preview_open = true;
            }
            
            for (int j = 0; j < 15 && rf.name[j]; j++) {
                draw_abstract_char(content_x + 40 + j * 8, iy + (item_h - 12) / 2, rf.name[j], VxTheme::TEXT_PRIMARY);
            }
            
            vxr_fill_rect(content_x + 40, iy + item_h - 1, content_w - 80, 1, VxTheme::BORDER_SUBTLE);
            drawn++;
        }
    }
}
