#pragma once

#include <stdint.h>
#include <stddef.h>

// ─── Calculator with decimal/floating-point support ─────────────────────────
// Uses string-based entry for display and scaled int64_t arithmetic
// (SCALE = 1000, giving 3 decimal places).
//
// calc_press(char key) remains the UNIFIED entry point for both keyboard and
// mouse, guaranteeing identical behavior between the two input methods.
//
// UI rebuilt on VXUI native framework — distinct button types, proper layout.

#define CALC_SCALE 1000
#define CALC_BUF_SZ 16

// ─── Local state ────────────────────────────────────────────────────────────
static char    calc_buf[CALC_BUF_SZ] = "0";
static int     calc_buf_len = 1;
static bool    calc_has_dot = false;
static int64_t calc_val = 0;
static int64_t calc_pending = 0;
static char    calc_op = 0;
static bool    calc_rep = false;
static bool    calc_err = false;

// ─── Helpers ────────────────────────────────────────────────────────────────
static int64_t parse_scaled(const char* buf, int len) {
    if (!buf || len <= 0) return 0;
    int64_t result = 0;
    int i = 0;
    bool neg = false;
    if (buf[0] == '-') { neg = true; i++; }
    int frac_digits = 0;
    bool in_frac = false;
    for (; i < len; i++) {
        if (buf[i] == '.') { in_frac = true; continue; }
        if (buf[i] >= '0' && buf[i] <= '9') {
            result = result * 10 + (buf[i] - '0');
            if (in_frac) frac_digits++;
        }
    }
    while (frac_digits < 3) { result *= 10; frac_digits++; }
    while (frac_digits > 3) { result /= 10; frac_digits--; }
    return neg ? -result : result;
}

static int format_scaled(int64_t val, char* buf, int max_len) {
    if (!buf || max_len < 2) return 0;
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
    bool neg = (val < 0);
    if (neg) val = -val;
    int64_t int_part = val / CALC_SCALE;
    int frac_part = (int)(val % CALC_SCALE);
    if (frac_part < 0) frac_part = -frac_part;
    char int_buf[16];
    int int_len = 0;
    if (int_part == 0) { int_buf[int_len++] = '0'; }
    else { while (int_part > 0 && int_len < 16) { int_buf[int_len++] = '0' + (int_part % 10); int_part /= 10; } }
    int pos = 0;
    if (neg && max_len > pos) buf[pos++] = '-';
    for (int i = int_len - 1; i >= 0 && pos < max_len - 1; i--) buf[pos++] = int_buf[i];
    if (frac_part > 0 && pos < max_len - 2) {
        buf[pos++] = '.';
        char frac_buf[4];
        frac_buf[0] = '0' + (frac_part / 100);
        frac_buf[1] = '0' + ((frac_part / 10) % 10);
        frac_buf[2] = '0' + (frac_part % 10);
        frac_buf[3] = 0;
        int f_len = 3;
        while (f_len > 0 && frac_buf[f_len - 1] == '0') f_len--;
        for (int i = 0; i < f_len && pos < max_len - 1; i++) buf[pos++] = frac_buf[i];
    }
    buf[pos] = 0;
    return pos;
}

static void rebuild_value() { calc_val = parse_scaled(calc_buf, calc_buf_len); }

// ─── Public API ─────────────────────────────────────────────────────────────
static void calc_clear() {
    calc_buf[0] = '0'; calc_buf[1] = 0;
    calc_buf_len = 1; calc_has_dot = false;
    calc_val = 0; calc_pending = 0;
    calc_op = 0; calc_rep = false; calc_err = false;
}

static void calc_press(char key) {
    if (key >= '0' && key <= '9') {
        if (calc_err || calc_rep) {
            calc_buf[0] = key; calc_buf[1] = 0; calc_buf_len = 1;
            calc_has_dot = false; calc_rep = false; calc_err = false;
            rebuild_value();
        } else if (calc_buf_len < CALC_BUF_SZ - 1) {
            calc_buf[calc_buf_len] = key; calc_buf[calc_buf_len + 1] = 0;
            calc_buf_len++; rebuild_value();
        }
    } else if (key == '.') {
        if (calc_rep || calc_err) {
            calc_buf[0] = '0'; calc_buf[1] = '.'; calc_buf[2] = 0;
            calc_buf_len = 2; calc_has_dot = true;
            calc_rep = false; calc_err = false; rebuild_value();
        } else if (!calc_has_dot && calc_buf_len < CALC_BUF_SZ - 1) {
            calc_buf[calc_buf_len] = '.'; calc_buf[calc_buf_len + 1] = 0;
            calc_buf_len++; calc_has_dot = true; rebuild_value();
        }
    } else if (key == 'C') {
        calc_clear();
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        if (calc_op) {
            if (calc_op == '+') calc_pending = calc_pending + calc_val;
            else if (calc_op == '-') calc_pending = calc_pending - calc_val;
            else if (calc_op == '*') calc_pending = calc_pending * calc_val / CALC_SCALE;
            else if (calc_op == '/') {
                if (calc_val == 0) { calc_err = true; return; }
                calc_pending = calc_pending * CALC_SCALE / calc_val;
            }
        } else calc_pending = calc_val;
        calc_val = calc_pending;
        calc_buf_len = format_scaled(calc_val, calc_buf, CALC_BUF_SZ);
        calc_has_dot = false;
        for (int i = 0; i < calc_buf_len; i++) if (calc_buf[i] == '.') { calc_has_dot = true; break; }
        calc_op = key; calc_rep = true;
    } else if (key == '=') {
        if (calc_op) {
            if (calc_op == '+') calc_val = calc_pending + calc_val;
            else if (calc_op == '-') calc_val = calc_pending - calc_val;
            else if (calc_op == '*') calc_val = calc_pending * calc_val / CALC_SCALE;
            else if (calc_op == '/') {
                if (calc_val == 0) { calc_err = true; return; }
                calc_val = calc_pending * CALC_SCALE / calc_val;
            }
            calc_pending = 0; calc_op = 0;
        }
        calc_buf_len = format_scaled(calc_val, calc_buf, CALC_BUF_SZ);
        calc_has_dot = false;
        for (int i = 0; i < calc_buf_len; i++) if (calc_buf[i] == '.') { calc_has_dot = true; break; }
        calc_rep = true;
    }
}

static void calc_handle_key(char c) {
    if (c >= '0' && c <= '9') calc_press(c);
    else if (c == '.') calc_press('.');
    else if (c == '+' || c == '-' || c == '*' || c == '/') calc_press(c);
    else if (c == '=' || c == '\n') calc_press('=');
    else if (c == '\b') {
        if (calc_buf_len > 1 && !calc_rep && !calc_err) {
            if (calc_buf[calc_buf_len - 1] == '.') calc_has_dot = false;
            calc_buf_len--; calc_buf[calc_buf_len] = 0; rebuild_value();
        } else if (calc_err) calc_clear();
    } else if (c == 'C' || c == 'c') calc_clear();
}

// ─── VXUI-powered Calculator UI ─────────────────────────────────────────────
// Include the VXUI framework (expected to be already included by the compositor)
// extern references come from vxui.hpp's forward declarations.

void draw_app_calculator(VxWindow& w, uint64_t /*frame*/, int mouse_x, int mouse_y, bool clicked) {

    int win_w = w.w;
    int win_h = w.h;
    int pad = VxTheme::SP_LG; // 16px padding

    // ── Display panel ───────────────────────────────────────────────────
    int disp_x = w.x + pad;
    int disp_y = w.y + 36; // below 32px title bar
    int disp_w = win_w - pad * 2;
    int disp_h = 56;

    VxPanel disp_panel = {disp_x, disp_y, disp_w, disp_h, 1}; // raised
    disp_panel.draw();

    // Display content — warm premium display, not neon
    int num_x = disp_x + disp_w - pad;
    int num_y = disp_y + 12;
    if (calc_err) {
        vxair_fb_fill_rect(disp_x + pad, disp_y + 12, 20, 30, VxTheme::DANGER);
        VxLabel err_lbl = {disp_x + pad + 28, disp_y + 22, "Error", VxTheme::DANGER, VxTheme::FONT_BODY};
        err_lbl.draw();
    } else {
        int digit_spacing = 20;
        int dx = num_x;
        uint32_t display_col = VxTheme::accent_glow(); // Dynamic accent, not neon
        for (int i = calc_buf_len - 1; i >= 0; i--) {
            char ch = calc_buf[i];
            if (ch >= '0' && ch <= '9') {
                draw_digit(dx, num_y, ch - '0', display_col);
                dx -= digit_spacing;
            } else if (ch == '.') {
                vxair_fb_fill_rect(dx + digit_spacing - 6, num_y + 20, 6, 6, display_col);
            } else if (ch == '-') {
                draw_segment(dx, num_y + 11, 12, true, display_col);
                dx -= 16;
            }
        }
    }

    // ── Button grid (5 rows × 4 cols) using VXUI components ────────────
    int grid_x = w.x + pad;
    int grid_y = disp_y + disp_h + VxTheme::SP_MD;
    int grid_w = disp_w;
    int btn_w = (grid_w - VxTheme::SP_SM * 3) / 4;
    int btn_h = 44;
    int gap = VxTheme::SP_SM;

    // Define all buttons with their variants
    struct { int col, row; const char* label; VxButtonVariant variant; } btns[] = {
        {0, 0, "7", VX_BTN_DIGIT}, {1, 0, "8", VX_BTN_DIGIT},
        {2, 0, "9", VX_BTN_DIGIT}, {3, 0, "/", VX_BTN_OPERATOR},
        {0, 1, "4", VX_BTN_DIGIT}, {1, 1, "5", VX_BTN_DIGIT},
        {2, 1, "6", VX_BTN_DIGIT}, {3, 1, "*", VX_BTN_OPERATOR},
        {0, 2, "1", VX_BTN_DIGIT}, {1, 2, "2", VX_BTN_DIGIT},
        {2, 2, "3", VX_BTN_DIGIT}, {3, 2, "-", VX_BTN_OPERATOR},
        {0, 3, "C", VX_BTN_UTILITY}, {1, 3, "0", VX_BTN_DIGIT},
        {2, 3, ".", VX_BTN_UTILITY}, {3, 3, "+", VX_BTN_OPERATOR},
        {3, 4, "=", VX_BTN_ACTION},
    };

    int num_btns = sizeof(btns) / sizeof(btns[0]);
    for (int i = 0; i < num_btns; i++) {
        int col = btns[i].col;
        int row = btns[i].row;
        int bx = grid_x + col * (btn_w + gap);
        int by = grid_y + row * (btn_h + gap);
        int bw = btn_w;
        // Action button (=) spans 1 column width
        if (btns[i].variant == VX_BTN_ACTION) {
            bw = btn_w;
        }

        VxButton btn = {bx, by, bw, btn_h, btns[i].label, btns[i].variant, false, false};
        btn.check_hover(mouse_x, mouse_y);
        btn.draw();

        // Handle click
        if (clicked && btn.is_hovered) {
            char key = 0;
            if (row == 0 && col == 3) key = '/';
            else if (row == 1 && col == 3) key = '*';
            else if (row == 2 && col == 3) key = '-';
            else if (row == 3 && col == 0) key = 'C';
            else if (row == 3 && col == 1) key = '0';
            else if (row == 3 && col == 2) key = '.';
            else if (row == 3 && col == 3) key = '+';
            else if (row == 4 && col == 3) key = '=';
            else if (row == 0) key = '7' + col;
            else if (row == 1) key = '4' + col;
            else if (row == 2) key = '1' + col;
            if (key) calc_press(key);
        }
    }
}
