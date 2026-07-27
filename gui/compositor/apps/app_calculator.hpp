#pragma once

#include <stdint.h>
#include <stddef.h>

// ─── Calculator with decimal/floating-point support ─────────────────────────
// Uses string-based entry for display and scaled int64_t arithmetic
// (SCALE = 1000, giving 3 decimal places). State is local static — the
// old g_state.calc_accumulator / calc_pending_value / calc_operator /
// calc_replace_display / calc_error fields become unused (but harmless).
//
// calc_press(char key) remains the UNIFIED entry point for both keyboard and
// mouse, guaranteeing identical behavior between the two input methods.

#define CALC_SCALE 1000
#define CALC_BUF_SZ 16

// ─── Local state (not in VxGuiState — safe because this header is
// included in only one translation unit, vxair_vxcomp.cpp) ────────────
static char calc_buf[CALC_BUF_SZ] = "0";
static int  calc_buf_len = 1;
static bool calc_has_dot = false;

// Scaled value of calc_buf (parse_scaled(calc_buf) * CALC_SCALE).
// Updated by rebuild_value() after every entry change.
static int64_t calc_val = 0;

static int64_t calc_pending = 0;
static char    calc_op = 0;          // '+', '-', '*', '/', or 0
static bool    calc_rep = false;     // next digit replaces display
static bool    calc_err = false;     // division-by-zero error

// ─── Helpers ────────────────────────────────────────────────────────────────

// Parse the display string into a scaled int64_t.
// "12.5" → 12500, "3" → 3000, "-0.5" → -500, "0" → 0.
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
    // Scale up to CALC_SCALE (fill missing decimal places)
    while (frac_digits < 3) { result *= 10; frac_digits++; }
    // Scale down if more than 3 decimal digits were entered
    while (frac_digits > 3) { result /= 10; frac_digits--; }
    return neg ? -result : result;
}

// Format a scaled int64_t into the display string.
// 12500 → "12.5", -500 → "-0.5", 0 → "0".
// Always uses up to 3 decimal places but strips trailing zeros after the dot.
// Returns the new length.
static int format_scaled(int64_t val, char* buf, int max_len) {
    if (!buf || max_len < 2) return 0;
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return 1; }

    bool neg = (val < 0);
    if (neg) val = -val;

    // Split into integer and fractional parts
    int64_t int_part = val / CALC_SCALE;
    int frac_part = (int)(val % CALC_SCALE);
    if (frac_part < 0) frac_part = -frac_part;

    // Build integer part (least-significant first)
    char int_buf[16];
    int int_len = 0;
    if (int_part == 0) {
        int_buf[int_len++] = '0';
    } else {
        while (int_part > 0 && int_len < 16) {
            int_buf[int_len++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }

    int pos = 0;
    if (neg && max_len > pos) buf[pos++] = '-';

    // Copy integer part (reversed)
    for (int i = int_len - 1; i >= 0 && pos < max_len - 1; i--) {
        buf[pos++] = int_buf[i];
    }

    // Fractional part (strip trailing zeros)
    if (frac_part > 0 && pos < max_len - 2) {
        buf[pos++] = '.';
        // frac_part is 0..999, pad to 3 digits then strip trailing zeros
        char frac_buf[4];
        frac_buf[0] = '0' + (frac_part / 100);
        frac_buf[1] = '0' + ((frac_part / 10) % 10);
        frac_buf[2] = '0' + (frac_part % 10);
        frac_buf[3] = 0;
        // Strip trailing zeros
        int f_len = 3;
        while (f_len > 0 && frac_buf[f_len - 1] == '0') f_len--;
        for (int i = 0; i < f_len && pos < max_len - 1; i++) {
            buf[pos++] = frac_buf[i];
        }
    }

    buf[pos] = 0;
    return pos;
}

// Rebuild calc_val from calc_buf (call after every entry change).
static void rebuild_value() {
    calc_val = parse_scaled(calc_buf, calc_buf_len);
}

// ─── Public API ─────────────────────────────────────────────────────────────

// Reset all calculator state (the "C" / Clear action).
static void calc_clear() {
    calc_buf[0] = '0'; calc_buf[1] = 0;
    calc_buf_len = 1;
    calc_has_dot = false;
    calc_val = 0;
    calc_pending = 0;
    calc_op = 0;
    calc_rep = false;
    calc_err = false;
}

// Unified "button press" — called by both keyboard (via calc_handle_key)
// and mouse click (via draw_app_calculator). key is the button label:
// '0'-'9', '.', '+', '-', '*', '/', '=', or 'C'.
static void calc_press(char key) {
    if (key >= '0' && key <= '9') {
        if (calc_err || calc_rep) {
            // Start new number
            calc_buf[0] = key;
            calc_buf[1] = 0;
            calc_buf_len = 1;
            calc_has_dot = false;
            calc_rep = false;
            calc_err = false;
            rebuild_value();
        } else {
            // Append digit (limit to 8 display chars, which allows e.g. 12345.67)
            if (calc_buf_len < CALC_BUF_SZ - 1) {
                calc_buf[calc_buf_len] = key;
                calc_buf[calc_buf_len + 1] = 0;
                calc_buf_len++;
                rebuild_value();
            }
        }
    } else if (key == '.') {
        if (calc_rep || calc_err) {
            // Start new number with "0."
            calc_buf[0] = '0';
            calc_buf[1] = '.';
            calc_buf[2] = 0;
            calc_buf_len = 2;
            calc_has_dot = true;
            calc_rep = false;
            calc_err = false;
            rebuild_value();
        } else if (!calc_has_dot) {
            // Append decimal point
            if (calc_buf_len < CALC_BUF_SZ - 1) {
                calc_buf[calc_buf_len] = '.';
                calc_buf[calc_buf_len + 1] = 0;
                calc_buf_len++;
                calc_has_dot = true;
                rebuild_value();
            }
        }
        // else: second decimal point is silently ignored (blocked)
    } else if (key == 'C') {
        calc_clear();
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        if (calc_op) {
            // Chain: evaluate previous operator first
            if (calc_op == '+') calc_pending = calc_pending + calc_val;
            else if (calc_op == '-') calc_pending = calc_pending - calc_val;
            else if (calc_op == '*') calc_pending = calc_pending * calc_val / CALC_SCALE;
            else if (calc_op == '/') {
                if (calc_val == 0) { calc_err = true; return; }
                calc_pending = calc_pending * CALC_SCALE / calc_val;
            }
        } else {
            calc_pending = calc_val;
        }
        calc_val = calc_pending;  // Display intermediate result
        // Sync display buffer
        calc_buf_len = format_scaled(calc_val, calc_buf, CALC_BUF_SZ);
        calc_has_dot = false;
        for (int i = 0; i < calc_buf_len; i++) {
            if (calc_buf[i] == '.') { calc_has_dot = true; break; }
        }
        calc_op = key;
        calc_rep = true;
    } else if (key == '=') {
        if (calc_op) {
            if (calc_op == '+') calc_val = calc_pending + calc_val;
            else if (calc_op == '-') calc_val = calc_pending - calc_val;
            else if (calc_op == '*') calc_val = calc_pending * calc_val / CALC_SCALE;
            else if (calc_op == '/') {
                if (calc_val == 0) { calc_err = true; return; }
                calc_val = calc_pending * CALC_SCALE / calc_val;
            }
            calc_pending = 0;
            calc_op = 0;
        }
        // Sync display buffer
        calc_buf_len = format_scaled(calc_val, calc_buf, CALC_BUF_SZ);
        calc_has_dot = false;
        for (int i = 0; i < calc_buf_len; i++) {
            if (calc_buf[i] == '.') { calc_has_dot = true; break; }
        }
        calc_rep = true;
    }
}

// Keyboard handler — maps ASCII chars to calc_press calls.
// Called from vxair_vxcomp.cpp's keyboard dispatch when Calculator is focused.
static void calc_handle_key(char c) {
    if (c >= '0' && c <= '9') {
        calc_press(c);
    } else if (c == '.') {
        calc_press('.');
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        calc_press(c);
    } else if (c == '=' || c == '\n') {
        calc_press('=');
    } else if (c == '\b') {
        // Backspace removes the last character from the entry buffer
        if (calc_buf_len > 1 && !calc_rep && !calc_err) {
            if (calc_buf[calc_buf_len - 1] == '.') calc_has_dot = false;
            calc_buf_len--;
            calc_buf[calc_buf_len] = 0;
            rebuild_value();
        } else if (calc_err) {
            calc_clear();
        }
        // If buf_len == 1, backspace shows "0"
        if (calc_buf_len == 1 && calc_buf[0] == '0') {
            // Already at zero, nothing to remove
        }
    } else if (c == 'C' || c == 'c') {
        calc_clear();
    }
    // Escape (27) is handled in the global keyboard dispatch
    // (vxair_vxcomp.cpp) scoped to Calculator — it never reaches here.
    // But even if it did, calc_clear() is correct.
}

// Draw the calculator window — display (from calc_buf) + 4×4 button grid.
void draw_app_calculator(VxWindow& w, uint64_t /*frame*/, int mouse_x, int mouse_y, bool clicked) {
    // ── Display background ──────────────────────────────────────────────
    vxair_fb_fill_rect(w.x + 20, w.y + 40, w.w - 40, 50, 0xFFE8F0F4);

    // ── Display digits (from calc_buf, not g_state) ─────────────────────
    if (calc_err) {
        // Error indicator
        vxair_fb_fill_rect(w.x + 30, w.y + 55, 15, 20, 0xFFE05050);
    } else {
        // Draw the display string, character by character, using 7-segment
        // digits for '0'-'9', a dot for '.', and a minus segment for '-'.
        int dx = w.x + 30;
        int dy = w.y + 55;
        for (int i = 0; i < calc_buf_len; i++) {
            char ch = calc_buf[i];
            if (ch >= '0' && ch <= '9') {
                draw_digit(dx, dy, ch - '0', 0xFF304050);
                dx += 20;
            } else if (ch == '.') {
                // Draw a small dot at the bottom right of the previous digit slot
                vxair_fb_fill_rect(dx - 6, dy + 18, 6, 6, 0xFF304050);
                // Don't advance X — the dot sits within the previous digit's space
            } else if (ch == '-') {
                draw_segment(dx, dy + 11, 12, true, 0xFF304050);
                dx += 16;
            }
        }
    }

    // ── Button grid (5×4) ───────────────────────────────────────────────
    // Rows 0-2 are standard digits + operators. Row 3: C 0 . + | Row 4: = (empty)
    // Button height reduced to 48px (from 60) to fit 5 rows within the
    // 390px window: start at y=110, end at 110+5*48+4*6=374 < 390.
    uint32_t bx = w.x + 20;
    uint32_t by = w.y + 110;
    uint32_t bw = (w.w - 50) / 4;
    uint32_t bh = 48;
    uint32_t gap = 6;

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            uint32_t cx = bx + c * (bw + gap);
            uint32_t cy = by + r * (bh + gap);
            bool hover = (mouse_x >= (int)cx && mouse_x <= (int)(cx + bw) &&
                          mouse_y >= (int)cy && mouse_y <= (int)(cy + bh));

            char key = 0;
            if (r == 0) {
                if      (c == 0) key = '7';
                else if (c == 1) key = '8';
                else if (c == 2) key = '9';
                else             key = '/';
            } else if (r == 1) {
                if      (c == 0) key = '4';
                else if (c == 1) key = '5';
                else if (c == 2) key = '6';
                else             key = '*';
            } else if (r == 2) {
                if      (c == 0) key = '1';
                else if (c == 1) key = '2';
                else if (c == 2) key = '3';
                else             key = '-';
            } else if (r == 3) {
                if      (c == 0) key = 'C';
                else if (c == 1) key = '0';
                else if (c == 2) key = '.';
                else             key = '+';
            } else if (r == 4) {
                // Row 4: '=' at column 3 (rightmost, under '+'), empty elsewhere
                if (c == 3) key = '=';
            }

            // Only draw and handle clicks for cells that have a key
            if (key != 0) {
                vxair_fb_fill_rect(cx, cy, bw, bh, hover ? 0xFFEAF1F5 : 0xFFF0F5F8);

                if (clicked && hover) {
                    calc_press(key);
                }

                // Draw the button label
                if (key >= '0' && key <= '9') {
                    draw_digit(cx + bw / 2 - 6, cy + bh / 2 - 11, key - '0', 0xFF607080);
                } else if (key == '.') {
                    vxair_fb_fill_rect(cx + bw / 2 - 4, cy + bh / 2 - 4, 8, 8, 0xFF607080);
                } else if (key == '-') {
                    draw_segment(cx + bw / 2 - 6, cy + bh / 2 - 1, 12, true, 0xFF607080);
                } else if (key == '=') {
                    draw_segment(cx + bw / 2 - 6, cy + bh / 2 - 4, 12, true, 0xFF607080);
                    draw_segment(cx + bw / 2 - 6, cy + bh / 2 + 2, 12, true, 0xFF607080);
                } else if (key == '+') {
                    draw_segment(cx + bw / 2 - 6, cy + bh / 2 - 1, 12, true, 0xFF607080);
                    draw_segment(cx + bw / 2 - 1, cy + bh / 2 - 6, 10, false, 0xFF607080);
                } else {
                    // Placeholder dot for 'C'
                    vxair_fb_fill_rect(cx + bw / 2 - 2, cy + bh / 2 - 2, 4, 4, 0xFF607080);
                }
            }
        }
    }
}
