#ifndef APP_SYSMON_HPP
#define APP_SYSMON_HPP

static void draw_app_sysmon(VxWindow& w, uint64_t /*frame*/, int mouse_x, int mouse_y, bool /*clicked*/) {
    uint32_t accent = VxTheme::accent();
    int margin = 28;
    int sm_x = w.x + margin;
    int sm_w = w.w - margin * 2;
    int row_h = 70;
    int bar_h = 18;

    struct Metric { const char* label; int pct; uint32_t color; };
    Metric metrics[] = {
        {"RAM",  45, accent},
        {"CPU",  15, VxTheme::SUCCESS},
        {"DISK", 30, VxTheme::WARNING},
    };
    int n = sizeof(metrics) / sizeof(metrics[0]);

    for (int i = 0; i < n; i++) {
        int cy = w.y + 44 + i * row_h;
        // Label
        VxLabel lbl = {sm_x, cy, metrics[i].label, VxTheme::TEXT_PRIMARY, VxTheme::FONT_BODY};
        lbl.draw();
        // Percentage
        char pct_str[5];
        int len = 0;
        int p = metrics[i].pct;
        if (p >= 100) { pct_str[len++] = '1'; pct_str[len++] = '0'; pct_str[len++] = '0'; }
        else { if (p >= 10) pct_str[len++] = '0' + p / 10; pct_str[len++] = '0' + p % 10; }
        pct_str[len++] = '%';
        pct_str[len] = 0;
        for (int j = 0; pct_str[j]; j++) {
            draw_abstract_char(sm_x + sm_w - (len - j) * 10, cy, pct_str[j], VxTheme::TEXT_SECONDARY);
        }
        // Bar background
        VxPanel bar_bg = {sm_x, cy + 20, sm_w, bar_h, 0};
        bar_bg.draw();
        // Bar fill
        int fill_w = (sm_w - 4) * metrics[i].pct / 100;
        vxair_fb_fill_rect(sm_x + 2, cy + 22, fill_w, bar_h - 4, metrics[i].color);
    }

    // Bottom stats grid
    int grid_y = w.y + 44 + n * row_h + 16;
    const char* stat_titles[4] = {"UPTIME", "PROCESSES", "THREADS", "HANDLES"};
    const char* stat_vals[4] = {"00:42:18", "12", "48", "156"};
    int card_w = (sm_w - 24) / 2;
    int card_h = 52;
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            int idx = r * 2 + c;
            int cx = sm_x + c * (card_w + 16);
            int cy = grid_y + r * (card_h + 16);
            VxPanel card = {cx, cy, card_w, card_h, 1};
            card.draw();
            VxLabel t = {cx + 12, cy + 10, stat_titles[idx], VxTheme::TEXT_SECONDARY, VxTheme::FONT_BODY};
            t.draw();
            VxLabel v = {cx + 12, cy + 28, stat_vals[idx], VxTheme::TEXT_PRIMARY, VxTheme::FONT_LARGE};
            v.draw();
        }
    }
}

#endif // APP_SYSMON_HPP
