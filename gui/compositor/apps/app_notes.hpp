#pragma once

#include <stdint.h>
#include "../vxair_textinput.hpp"

static char notes_buffer[1024] = {0};
static int notes_len = 0;
static VxTextInput notes_input = { notes_buffer, &notes_len, 1024, 0, 0 };

static uint64_t last_notes_click_frame = 1000000;

static void notes_handle_key(char c) {
    // Clipboard codes (26=copy, 28=cut, 29=paste) from Ctrl+C/X/V
    if (c == 26) { vx_copy(notes_input); return; }
    if (c == 28) { vx_cut(notes_input); return; }
    if (c == 29) { vx_paste(notes_input); return; }
    if (notes_input.handle_key(c)) {
        // Handled by shared text-input module!
    } else if (c == '\n') {
        // Multi-line newline insert at caret, via the shared primitive.
        notes_input.insert_char('\n');
    }
}

void draw_app_notes(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int margin_x = w.x + 40;
    int top_y = w.y + 40;
    int wrap_x = w.x + w.w - 20;
    int line_h = 28;
    int char_w = 12;
    uint32_t accent = VxTheme::accent();
    uint32_t text_col = VxTheme::TEXT_PRIMARY;
    uint32_t sel_color = 0xFF334155;

    // Panel wrapping the note content
    VxPanel note_area = {w.x + 4, w.y + 30, w.w - 8, w.h - 34, 0};
    note_area.draw();

    vxair_fb_fill_rect(w.x + 30, w.y + 28, 2, w.h - 30, accent); // Margin accent
    for (int i = 0; i < 13; i++) {
        vxair_fb_fill_rect(w.x + 2, w.y + 60 + i * 28, w.w - 4, 1, VxTheme::BORDER_SUBTLE);
    }

    // ── Click-to-position + double-click-select-all (multi-line) ─────────
    if (clicked && mouse_x >= w.x + 30 && mouse_x <= wrap_x &&
        mouse_y >= top_y) {
        if (last_notes_click_frame != 1000000 && frame >= last_notes_click_frame &&
            frame - last_notes_click_frame < 25) {
            notes_input.select_all();
            last_notes_click_frame = 1000000;
        } else {
            int idx = vx_wrapped_index_at(notes_buffer, notes_len,
                                          margin_x, top_y, wrap_x,
                                          line_h, char_w,
                                          mouse_x, mouse_y);
            notes_input.set_caret(idx);
            last_notes_click_frame = frame;
        }
    }

    int cur_x = margin_x;
    int cur_y = top_y;
    // Track caret pixel position separately so the caret is drawn AFTER all
    // characters (otherwise the glyph at the caret's index overwrites the
    // 2px caret, making it invisible when caret_pos is mid-buffer).
    int caret_px = cur_x;
    int caret_py = cur_y;
    for (int i = 0; i <= notes_len; i++) {
        if (i == notes_input.caret_pos) {
            caret_px = cur_x;
            caret_py = cur_y;
        }
        if (i == notes_len) break;

        if (notes_buffer[i] == '\n') {
            if (notes_input.sel_active() && i >= notes_input.sel_min() && i < notes_input.sel_max()) {
                vxair_fb_fill_rect(cur_x, cur_y - 2, 8, 18, sel_color);
            }
            cur_x = margin_x;
            cur_y += line_h;
        } else {
            if (notes_input.sel_active() && i >= notes_input.sel_min() && i < notes_input.sel_max()) {
                vxair_fb_fill_rect(cur_x, cur_y - 2, char_w, 18, sel_color);
            }
            draw_abstract_char(cur_x, cur_y, notes_buffer[i], text_col);
            cur_x += char_w;
            if (cur_x > wrap_x) {
                cur_x = margin_x;
                cur_y += line_h;
            }
        }
    }
    if (w.focused && (frame % 60 < 30) && !notes_input.sel_active()) {
        vxair_fb_fill_rect(caret_px, caret_py, 2, 14, accent);
    }
}
