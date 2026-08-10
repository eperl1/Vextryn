#ifndef APP_TERMINAL_HPP
#define APP_TERMINAL_HPP

#include "../vxair_textinput.hpp"

static VxTextInput term_input;

static void bind_term_input() {
    term_input.buffer = g_state.term_buffer;
    term_input.len = &g_state.term_len;
    term_input.max_len = 64;
}

static void terminal_handle_key(char c) {
    bind_term_input();
    if (c == 26) { vx_copy(term_input); return; }
    if (c == 28) { vx_cut(term_input); return; }
    if (c == 29) { vx_paste(term_input); return; }
    if (c == '\n') {
        g_state.term_len = 0;
        g_state.term_buffer[0] = 0;
        term_input.caret_pos = 0;
        term_input.selection_anchor = 0;
        return;
    }
    term_input.handle_key(c);
}

static void term_line(int x, int y, const char* s, uint32_t col) {
    for (int i = 0; s[i]; i++) draw_abstract_char(x + i * 8, y, s[i], col);
}

static void draw_app_terminal(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame;
    (void)mouse_x;
    (void)mouse_y;
    (void)clicked;
    bind_term_input();

    uint32_t bg = 0xFF151515;
    uint32_t fg = 0xFFE8E8E8;
    uint32_t cyan = 0xFF00D8F5;
    uint32_t dim = 0xFF8C8F94;

    vxr_fill_rect(w.x, w.y + 30, w.w, w.h - 30, bg);
    vxr_fill_rect(w.x + 14, w.y + 44, w.w - 28, 1, 0x22FFFFFF);

    int x = w.x + 18;
    int y = w.y + 56;
    term_line(x, y, "vextryn@air:~", cyan);
    term_line(x + 104, y, " $ uptime", fg);

    y += 28;
    term_line(x, y, "10:14 up 2 days, 4:11,  2 users,  load average: 0.42, 0.31, 0.24", fg);

    y += 28;
    term_line(x, y, "vextryn@air:~", cyan);
    term_line(x + 104, y, " $ ls -la", fg);

    y += 28;
    term_line(x + 8, y, "-rw-r--r--  vextryn  vextryn   812B  Jul 31 11:20  README.md", fg);
    y += 26;
    term_line(x + 8, y, "-rw-r--r--  vextryn  vextryn   2.1K  Jul 31 11:21  shell.cpp", fg);
    y += 26;
    term_line(x + 8, y, "drwxr-xr-x  vextryn  vextryn   4.0K  Jul 31 11:22  apps", fg);
    y += 26;
    term_line(x + 8, y, "-rw-r--r--  vextryn  vextryn   1.3K  Jul 31 11:23  config.h", fg);

    y += 34;
    term_line(x, y, "vextryn@air:~", cyan);
    term_line(x + 104, y, " $ ./build", fg);

    y += 30;
    term_line(x + 10, y, "[1/4] Compiling shell.cpp", fg);
    term_line(x + 228, y, "... done", dim);
    y += 26;
    term_line(x + 10, y, "[2/4] Linking air-shell", fg);
    term_line(x + 220, y, "... done", dim);

    int status_y = w.y + w.h - 34;
    vxr_fill_rect(w.x + 16, status_y, w.w - 32, 1, 0x18FFFFFF);
    term_line(w.x + 18, status_y + 12, "vextryn@air:~", cyan);
    term_line(w.x + 122, status_y + 12, " $ ", fg);
    if ((frame % 60) < 30) vxr_fill_rect(w.x + 146, status_y + 6, 8, 14, cyan);
}

#endif
