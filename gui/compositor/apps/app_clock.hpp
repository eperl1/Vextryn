#pragma once
// V5 Clock — Analog clock face with digital readout
#include <stdint.h>

static void draw_app_clock(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    // Compute clock geometry to fit entirely within the window content area.
    // Layout: title bar (38px) → padding → clock circle → gap → digital readout → padding
    int title_h   = VxTheme::TITLE_BAR_H;  // 38
    int pad        = 14;
    int readout_h  = 50;   // space reserved for the digital readout below the clock
    int avail_h    = w.h - title_h - pad * 2;     // total content height
    int clock_h    = avail_h - readout_h;          // height for the clock circle
    int clock_w    = w.w - pad * 2;                // width for the clock circle
    if (clock_h < 60) clock_h = 60;
    if (clock_w < 60) clock_w = 60;

    int cx = w.x + w.w / 2;
    int cy = w.y + title_h + pad + clock_h / 2;
    int radius = clock_h / 2;
    if (radius > clock_w / 2) radius = clock_w / 2;  // also constrain by width
    if (radius > 90) radius = 90;                     // aesthetic cap
    if (radius < 30) radius = 30;
    uint32_t accent = VxTheme::accent();

    // Clock face background — frosted glass circle
    for (int dy = -radius; dy <= radius; dy++) {
        int half = 0;
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) half = dx;
        }
        if (half >= 0)
            vxr_fill_rect(cx - half, cy + dy, half * 2 + 1, 1, VxTheme::SURFACE_HIGH);
    }
    // Outer ring
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist = dx * dx + dy * dy;
            if (dist <= radius * radius && dist > (radius - 3) * (radius - 3)) {
                vxr_fill_rect(cx + dx, cy + dy, 1, 1, VxTheme::BORDER_BRIGHT);
            }
        }
    }

    // Hour marks (12 ticks) — all integer math, no floats
    for (int h = 0; h < 12; h++) {
        // Precomputed sin/cos for 30-degree steps (×100)
        static const int sin30[12] = {0, 50, 87, 100, 87, 50, 0, -50, -87, -100, -87, -50};
        static const int cos30[12] = {100, 87, 50, 0, -50, -87, -100, -87, -50, 0, 50, 87};
        int tick_r = radius - 8;
        int tx = cx + (sin30[h] * tick_r) / 100;
        int ty = cy - (cos30[h] * tick_r) / 100;
        int inner_r = radius - 14;
        int ix = cx + (sin30[h] * inner_r) / 100;
        int iy = cy - (cos30[h] * inner_r) / 100;
        // Draw tick line (thick for 12/3/6/9, thin for others)
        int thickness = (h % 3 == 0) ? 3 : 1;
        uint32_t tcol = (h % 3 == 0) ? accent : VxTheme::TEXT_MUTED;
        // Draw line from inner to outer
        int steps = 8;
        for (int s = 0; s <= steps; s++) {
            int px = ix + (tx - ix) * s / steps;
            int py = iy + (ty - iy) * s / steps;
            vxr_fill_rect(px - thickness / 2, py - thickness / 2, thickness, thickness, tcol);
        }
        // Hour numbers at cardinal positions
        if (h % 3 == 0) {
            int nr = radius - 22;
            int nx = cx + (sin30[h] * nr) / 100;
            int ny = cy - (cos30[h] * nr) / 100 - 6;
            char num[2];
            int hour = (h == 0) ? 12 : h;
            num[0] = '0' + hour / 10 ? '0' + hour / 10 : ' ';
            num[0] = (hour >= 10) ? '1' : '0' + hour;
            num[1] = (hour >= 10) ? '0' + hour % 10 : 0;
            for (int i = 0; i < 2 && num[i]; i++)
                draw_abstract_char(nx - 4 + i * 8, ny, num[i], VxTheme::TEXT_SECONDARY);
        }
    }

    // Animated hands — use frame counter for smooth movement
    // Second hand: rotates every second (60 frames per second → 1 rev per 60s)
    int sec_angle = (frame / 60) % 60; // 0-59
    int min_angle = (frame / 3600) % 60; // 0-59
    int hour_angle = (frame / 21600) % 12; // 0-11

    // Full 60-entry sin/cos lookup for 6-degree steps (×100)
    // Correct sine curve: peaks at index 15 (90°), zero at 0°/30°, negative 31°-58°
    static const int sin6[60] = {
        0,10,21,31,41,50,59,67,74,81,87,91,95,98,99,100,
        99,98,95,91,87,81,74,67,59,50,41,31,21,10,0,-10,
        -21,-31,-41,-50,-59,-67,-74,-81,-87,-91,-95,-98,-99,-100,-99,-98,
        -95,-91,-87,-81,-74,-67,-59,-50,-41,-31,-21,-10
    };
    static const int cos6[60] = {
        100,99,98,95,91,87,81,74,67,59,50,41,31,21,10,0,
        -10,-21,-31,-41,-50,-59,-67,-74,-81,-87,-91,-95,-98,-99,-100,-99,
        -98,-95,-91,-87,-81,-74,-67,-59,-50,-41,-31,-21,-10,0,10,21,
        31,41,50,59,67,74,81,87,91,95,98,99
    };

    // Hour hand (short, thick)
    int hlen = radius * 50 / 100;
    int hx2 = cx + (sin6[hour_angle * 5 % 60] * hlen) / 100;
    int hy2 = cy - (cos6[hour_angle * 5 % 60] * hlen) / 100;
    for (int s = 0; s <= 20; s++) {
        int px = cx + (hx2 - cx) * s / 20;
        int py = cy + (hy2 - cy) * s / 20;
        vxr_fill_rect(px - 1, py - 1, 3, 3, VxTheme::TEXT_PRIMARY);
    }

    // Minute hand (medium, medium thickness)
    int mlen = radius * 75 / 100;
    int mx2 = cx + (sin6[min_angle] * mlen) / 100;
    int my2 = cy - (cos6[min_angle] * mlen) / 100;
    for (int s = 0; s <= 20; s++) {
        int px = cx + (mx2 - cx) * s / 20;
        int py = cy + (my2 - cy) * s / 20;
        vxr_fill_rect(px - 1, py, 2, 2, VxTheme::TEXT_PRIMARY);
    }

    // Second hand (long, thin, accent color)
    int slen = radius * 85 / 100;
    int sx2 = cx + (sin6[sec_angle] * slen) / 100;
    int sy2 = cy - (cos6[sec_angle] * slen) / 100;
    for (int s = 0; s <= 20; s++) {
        int px = cx + (sx2 - cx) * s / 20;
        int py = cy + (sy2 - cy) * s / 20;
        vxr_fill_rect(px, py, 1, 1, accent);
    }

    // Center dot
    vxr_fill_rect(cx - 3, cy - 3, 6, 6, accent);
    vxr_fill_rect(cx - 1, cy - 1, 2, 2, VxTheme::TEXT_PRIMARY);

    // Digital readout below clock — positioned in the reserved readout area
    int dy = w.y + w.h - pad - readout_h + 8;
    // Format: HH:MM:SS
    int hh = (frame / 21600) % 24; // Just use frame-based time
    int mm = (frame / 3600) % 60;
    int ss = (frame / 60) % 60;
    char dig[9];
    dig[0] = '0' + hh / 10; dig[1] = '0' + hh % 10; dig[2] = ':';
    dig[3] = '0' + mm / 10; dig[4] = '0' + mm % 10; dig[5] = ':';
    dig[6] = '0' + ss / 10; dig[7] = '0' + ss % 10; dig[8] = 0;
    int dw = 9 * 16; // 9 chars × 16px wide for big text
    int dx = cx - dw / 2;
    // Draw large-style: use segment display for digits, chars for colons
    for (int i = 0; dig[i]; i++) {
        if (dig[i] >= '0' && dig[i] <= '9') {
            draw_digit(dx + i * 24, dy, dig[i] - '0', VxTheme::TEXT_PRIMARY);
        } else if (dig[i] == ':') {
            vxr_fill_rect(dx + i * 24 + 4, dy + 4, 4, 4, VxTheme::accent());
            vxr_fill_rect(dx + i * 24 + 4, dy + 14, 4, 4, VxTheme::accent());
        }
    }
}
