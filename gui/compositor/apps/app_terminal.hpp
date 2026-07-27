#ifndef APP_TERMINAL_HPP
#define APP_TERMINAL_HPP

#include "../vxair_textinput.hpp"

// ─── Terminal input line bound to the shared text-input module ─────────────
// Bound to g_state.term_buffer / g_state.term_len so the command input gets
// the same caret, selection, Home/End, Shift+arrow, and click-to-position
// polish as Browser, Notes, and Files — all editing handled by VxTextInput
// (no duplicated logic). Command parsing/execution is preserved verbatim
// from the original inline code; only the input-editing wrapper changed.
static VxTextInput term_input;            // bound to term_buffer / term_len
static uint64_t last_term_click_frame = 1000000;

// (Re)bind the shared input to the global terminal buffer. Idempotent and
// safe to call every frame; does not reset caret/selection.
static void bind_term_input() {
    term_input.buffer = g_state.term_buffer;
    term_input.len = &g_state.term_len;
    term_input.max_len = 64;
}

// Keyboard dispatch for the Terminal. Replaces the inline logic that was in
// vxair_vxcomp.cpp. Input editing (caret/selection/navigation/insert) is
// delegated to VxTextInput — this also fixes the old bug where arrow-key
// codes (17-24) were appended as raw garbage chars to term_buffer. Command
// parsing/execution is preserved EXACTLY as the original (verbatim move).
static void terminal_handle_key(char c) {
    bind_term_input();

    // Clipboard codes (26=copy, 28=cut, 29=paste) from Ctrl+C/X/V
    if (c == 26) { vx_copy(term_input); return; }
    if (c == 28) { vx_cut(term_input); return; }
    if (c == 29) { vx_paste(term_input); return; }

    if (c == '\n') {
        // ── Execute command (verbatim from original inline code) ─────────
        g_state.term_buffer[g_state.term_len] = 0;
        if (g_state.term_len > 0) {
            if (g_state.term_buffer[0] == 'h') {
                const char* msg = "cmds: help, clear, whoami, version, date";
                for(int i=0; msg[i]&&i<511; i++) g_state.term_output[i] = msg[i];
                g_state.term_out_len = 40;
            } else if (g_state.term_buffer[0] == 'c') {
                g_state.term_out_len = 0;
            } else if (g_state.term_buffer[0] == 'w') {
                const char* msg = "vxair-root";
                for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                g_state.term_out_len = 10;
            } else if (g_state.term_buffer[0] == 'v') {
                const char* msg = "VextrynAir OS v0.1";
                for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                g_state.term_out_len = 18;
            } else if (g_state.term_buffer[0] == 'd') {
                const char* msg = "Mon Jul 20 2026";
                for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                g_state.term_out_len = 15;
            } else {
                const char* msg = "cmd not found";
                for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                g_state.term_out_len = 13;
            }
        }
        g_state.term_len = 0;
        g_state.term_buffer[0] = 0;
        // Reset caret/selection for the next command line.
        term_input.caret_pos = 0;
        term_input.selection_anchor = 0;
    } else {
        // All other keys (printable, Backspace, arrows, Home/End, Shift
        // variants) go through the shared module. Codes it doesn't handle
        // (e.g. Esc=27, Up/Down=0) are silently dropped — matching the
        // original behavior of ignoring unmapped keys, without the
        // garbage-char insertion bug.
        term_input.handle_key(c);
    }
}

static void draw_app_terminal(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    bind_term_input();

    uint32_t bg = 0xFF000000;
    uint32_t text_color = 0xFF00FF00;
    uint32_t sel_color = 0xFF1E3A1E; // subtle green-tinted selection

    vxair_fb_fill_rect(w.x, w.y + 28, w.w, w.h - 28, bg);

    int cur_x = w.x + 10;
    int cur_y = w.y + 40;
    int char_w = 10;

    const char* prompt = "vxair@root:~$ ";
    for (int i=0; prompt[i]; i++) {
        draw_abstract_char(cur_x, cur_y, prompt[i], text_color);
        cur_x += char_w;
    }
    // Input line starts after the prompt.
    int text_x = cur_x;
    int text_y = cur_y;

    // ── Click-to-position + double-click-select-all ──────────────────────
    if (clicked && mouse_x >= text_x && mouse_y >= text_y - 4 &&
        mouse_y <= text_y + 16) {
        if (last_term_click_frame != 1000000 && frame >= last_term_click_frame &&
            frame - last_term_click_frame < 25) {
            term_input.select_all();
            last_term_click_frame = 1000000;
        } else {
            int col = (mouse_x - text_x + char_w / 2) / char_w;
            if (col < 0) col = 0;
            if (col > g_state.term_len) col = g_state.term_len;
            term_input.set_caret(col);
            last_term_click_frame = frame;
        }
    }

    // ── Selection highlight (single-line) ────────────────────────────────
    if (term_input.sel_active()) {
        int s = term_input.sel_min();
        int e = term_input.sel_max();
        if (s < 0) s = 0;
        if (e > g_state.term_len) e = g_state.term_len;
        if (e > s) {
            vxair_fb_fill_rect(text_x + s * char_w, text_y - 1,
                               (e - s) * char_w, 16, sel_color);
        }
    }

    // ── Input characters ─────────────────────────────────────────────────
    for (int i=0; i<g_state.term_len; i++) {
        draw_abstract_char(text_x + i * char_w, text_y,
                           g_state.term_buffer[i], text_color);
    }

    // ── Caret (drawn AFTER chars so it isn't overwritten) ────────────────
    if (w.focused && (frame % 60 < 30) && !term_input.sel_active()) {
        vxair_fb_fill_rect(text_x + term_input.caret_pos * char_w, text_y,
                           8, 14, text_color);
    }

    // ── Command output (unchanged) ───────────────────────────────────────
    if (g_state.term_out_len > 0) {
        cur_y += 20;
        cur_x = w.x + 10;
        for (int i=0; i<g_state.term_out_len; i++) {
            draw_abstract_char(cur_x, cur_y, g_state.term_output[i], 0xFFAAAAAA);
            cur_x += char_w;
        }
    }
}

#endif
