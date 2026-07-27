#ifndef APP_FILE_MANAGER_HPP
#define APP_FILE_MANAGER_HPP

#include "../vxair_textinput.hpp"

// ─── Shared text-input bindings for the Files app ──────────────────────────
// Rebound to the currently-selected file's buffers whenever rename or
// content-edit mode is entered, so there is zero per-file state overhead.
// Both text fields get the same caret, selection, Home/End, and
// click-to-position polish as Browser and Notes — all editing logic is
// handled by the shared VxTextInput module (no duplication).
static VxTextInput file_name_input;      // bound to ram_files[idx].name
static VxTextInput file_content_input;   // bound to ram_files[idx].content
static int file_name_len = 0;            // kept in sync with name null-terminator
static uint64_t last_name_click_frame = 1000000;
static uint64_t last_content_click_frame = 1000000;

// Bind the name input to a file's name buffer and sync the length counter
// (name is null-terminated with no separate length field, so we derive it).
static void bind_name_input(RamFile& rf) {
    file_name_len = 0;
    while (rf.name[file_name_len] && file_name_len < 15) file_name_len++;
    file_name_input.buffer = rf.name;
    file_name_input.len = &file_name_len;
    file_name_input.max_len = 16;
}

// Bind the content input to a file's content buffer (content_len is native).
static void bind_content_input(RamFile& rf) {
    file_content_input.buffer = rf.content;
    file_content_input.len = &rf.content_len;
    file_content_input.max_len = 512;
}

// Keyboard dispatch for the Files app. Replaces the inline logic that was in
// vxair_vxcomp.cpp. Handles the rename field (single-line, Enter confirms)
// and the content editor (multi-line, Enter inserts newline). All
// caret/selection/navigation is delegated to VxTextInput — this also fixes
// the old bug where arrow-key codes (17-24) were inserted as garbage chars.
static void file_handle_key(char c) {
    if (g_state.file_selected_idx < 0 || g_state.file_selected_idx >= 10) return;
    RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
    if (!rf.in_use) return;

    if (g_state.file_rename_mode) {
        bind_name_input(rf);
        // Clipboard codes (26=copy, 28=cut, 29=paste)
        if (c == 26) { vx_copy(file_name_input); return; }
        if (c == 28) { vx_cut(file_name_input); save_files_to_disk(); return; }
        if (c == 29) { vx_paste(file_name_input); save_files_to_disk(); return; }
        if (c == '\n') {
            // Enter confirms the rename and exits rename mode.
            g_state.file_rename_mode = false;
            file_name_input.selection_anchor = file_name_input.caret_pos;
            save_files_to_disk();
        } else if (file_name_input.handle_key(c)) {
            save_files_to_disk();
        }
    } else if (g_state.file_preview_open) {
        bind_content_input(rf);
        // Clipboard codes (26=copy, 28=cut, 29=paste)
        if (c == 26) { vx_copy(file_content_input); return; }
        if (c == 28) { vx_cut(file_content_input); save_files_to_disk(); return; }
        if (c == 29) { vx_paste(file_content_input); save_files_to_disk(); return; }
        if (file_content_input.handle_key(c)) {
            save_files_to_disk();
        } else if (c == '\n') {
            // Insert newline at caret via the shared primitive (multi-line).
            if (file_content_input.insert_char('\n')) {
                save_files_to_disk();
            }
        }
    }
}

static void draw_app_file_manager(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t bg = 0xFF1E293B;
    uint32_t sidebar = 0xFF0F172A;
    uint32_t text_color = 0xFFF8FAFC;
    uint32_t accent = g_state.accent_color;
    uint32_t danger = 0xFFFF003C;
    uint32_t sel_color = 0xFF3E4451;

    // Sidebar
    vxair_fb_fill_rect(w.x, w.y + 28, 120, w.h - 28, sidebar);

    // Disk label
    draw_abstract_char(w.x + 10, w.y + 40, 'D', accent);
    draw_abstract_char(w.x + 22, w.y + 40, 'A', accent);
    draw_abstract_char(w.x + 34, w.y + 40, 'T', accent);
    draw_abstract_char(w.x + 46, w.y + 40, 'A', accent);

    // New File Button
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
                bind_content_input(g_state.ram_files[i]);
                file_content_input.caret_pos = 0;
                file_content_input.selection_anchor = 0;
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
        // ── Preview / Edit mode ────────────────────────────────────────
        RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
        // Ensure inputs are bound to this file (for rendering caret/selection).
        bind_name_input(rf);
        bind_content_input(rf);

        // Toolbar
        vxair_fb_fill_rect(w.x + 121, w.y + 28, w.w - 121, 40, sidebar);
        vxair_fb_fill_rect(w.x + 121, w.y + 67, w.w - 121, 1, 0xFF334155);

        // Back button
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
            if (g_state.file_rename_mode) {
                // Entering rename mode: select entire name for easy replacement.
                bind_name_input(rf);
                file_name_input.select_all();
            } else {
                file_name_input.selection_anchor = file_name_input.caret_pos;
            }
        }

        // Delete Button
        bool del_hover = (mouse_x >= w.x + w.w - 70 && mouse_x <= w.x + w.w - 10 && mouse_y >= w.y + 34 && mouse_y <= w.y + 60);
        vxair_fb_fill_rect(w.x + w.w - 70, w.y + 34, 60, 26, del_hover ? danger : 0xFF334155);
        vxair_fb_fill_rect(w.x + w.w - 69, w.y + 35, 58, 24, sidebar);
        draw_abstract_char(w.x + w.w - 46, w.y + 40, 'D', del_hover ? danger : text_color);
        if (clicked && del_hover) {
            rf.in_use = false;
            g_state.file_preview_open = false;
            g_state.file_rename_mode = false;
            save_files_to_disk();
        }

        // ─── Name field (single-line, VxTextInput-wired) ───────────────
        int nx = w.x + 250;
        int ny = w.y + 40;
        int name_char_w = 12;
        int name_field_end = w.x + w.w - 80;
        if (name_field_end < nx + 20) name_field_end = nx + 20;

        // Click-to-position + double-click-select-all for name field
        if (g_state.file_rename_mode && clicked &&
            mouse_x >= nx - 5 && mouse_x <= name_field_end &&
            mouse_y >= ny - 4 && mouse_y <= ny + 18 &&
            !back_hover && !rename_hover && !del_hover) {
            if (last_name_click_frame != 1000000 && frame >= last_name_click_frame &&
                frame - last_name_click_frame < 25) {
                file_name_input.select_all();
                last_name_click_frame = 1000000;
            } else {
                int col = (mouse_x - nx + name_char_w / 2) / name_char_w;
                if (col < 0) col = 0;
                if (col > file_name_len) col = file_name_len;
                file_name_input.set_caret(col);
                last_name_click_frame = frame;
            }
        }

        // Selection highlight for name
        if (g_state.file_rename_mode && file_name_input.sel_active()) {
            int s = file_name_input.sel_min();
            int e = file_name_input.sel_max();
            if (s < 0) s = 0;
            if (e > file_name_len) e = file_name_len;
            if (e > s) {
                vxair_fb_fill_rect(nx + s * name_char_w, ny - 2,
                                   (e - s) * name_char_w, 18, sel_color);
            }
        }

        // Name characters
        for (int c = 0; c < file_name_len; c++) {
            draw_abstract_char(nx + c * name_char_w, ny, rf.name[c],
                               g_state.file_rename_mode ? accent : text_color);
        }

        // Name caret (drawn AFTER chars so the glyph doesn't overwrite it)
        if (g_state.file_rename_mode && w.focused && (frame % 60 < 30) &&
            !file_name_input.sel_active()) {
            vxair_fb_fill_rect(nx + file_name_input.caret_pos * name_char_w, ny,
                               2, 14, accent);
        }

        // ─── Content editor (multi-line, VxTextInput-wired) ────────────
        int csx = w.x + 135;
        int csy = w.y + 80;
        int cwx = w.x + w.w - 20;
        int clh = 20;
        int ccw = 12;

        // Click-to-position + double-click-select-all for content
        if (!g_state.file_rename_mode && clicked && mouse_y >= csy &&
            mouse_x >= csx && mouse_x <= cwx) {
            if (last_content_click_frame != 1000000 && frame >= last_content_click_frame &&
                frame - last_content_click_frame < 25) {
                file_content_input.select_all();
                last_content_click_frame = 1000000;
            } else {
                int idx = vx_wrapped_index_at(rf.content, rf.content_len,
                                              csx, csy, cwx, clh, ccw,
                                              mouse_x, mouse_y);
                file_content_input.set_caret(idx);
                last_content_click_frame = frame;
            }
        }

        // Content rendering with selection highlight + caret-after-chars
        int cx = csx;
        int cy = csy;
        int caret_cx = cx, caret_cy = cy;
        for (int i = 0; i <= rf.content_len; i++) {
            if (i == file_content_input.caret_pos) {
                caret_cx = cx;
                caret_cy = cy;
            }
            if (i == rf.content_len) break;
            bool in_sel = file_content_input.sel_active() &&
                          i >= file_content_input.sel_min() &&
                          i < file_content_input.sel_max();
            if (rf.content[i] == '\n') {
                if (in_sel) vxair_fb_fill_rect(cx, cy - 2, 8, 18, sel_color);
                cx = csx;
                cy += clh;
            } else {
                if (in_sel) vxair_fb_fill_rect(cx, cy - 2, ccw, 18, sel_color);
                draw_abstract_char(cx, cy, rf.content[i], text_color);
                cx += ccw;
                if (cx > cwx) {
                    cx = csx;
                    cy += clh;
                }
            }
        }
        if (!g_state.file_rename_mode && w.focused && (frame % 60 < 30) &&
            !file_content_input.sel_active()) {
            vxair_fb_fill_rect(caret_cx, caret_cy, 2, 14, text_color);
        }

    } else {
        // ── List mode ──────────────────────────────────────────────────
        int list_y = w.y + 40;
        bool any = false;
        for (int i=0; i<10; i++) {
            if (g_state.ram_files[i].in_use) {
                any = true;
                bool item_hover = (mouse_x >= w.x + 130 && mouse_x <= w.x + w.w - 20 && mouse_y >= list_y && mouse_y <= list_y + 36);

                if (item_hover) vxair_fb_fill_rect(w.x + 130, list_y, w.w - 150, 36, 0xFF334155);

                // Icon (wireframe document)
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
                    bind_content_input(g_state.ram_files[i]);
                    file_content_input.caret_pos = g_state.ram_files[i].content_len;
                    file_content_input.selection_anchor = file_content_input.caret_pos;
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
