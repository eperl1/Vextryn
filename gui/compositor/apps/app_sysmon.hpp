#pragma once

static void sysmon_metric(int x, int y, int w, const char* label, const char* value, int pct) {
    vx_text::draw(x, y + 10, 11, label, VxTheme::FG_SOFT, VxTheme::BASE_DEEP);
    vx_text::draw(x + w - vx_text::text_width(11, value), y + 10, 11, value, VxTheme::FG_SOFT, VxTheme::BASE_DEEP);
    vxr_fill_rect(x, y + 24, w, 3, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 24, (w * pct) / 100, 3, 0xFF0FE7FF);
}

static void draw_app_sysmon(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    (void)frame;
    (void)mouse_x;
    (void)mouse_y;
    (void)clicked;

    int x = w.x + 10;
    int y = w.y + 38;
    int inner_w = w.w - 20;

    vxr_fill_rect(w.x, w.y + 30, w.w, w.h - 30, 0xFF171717);

    vx_text::draw(x, y + 4, 11, "CPU", VxTheme::FG_SOFT, 0xFF171717);
    vx_text::draw(x + inner_w - 26, y + 4, 11, "12%", VxTheme::FG_SOFT, 0xFF171717);
    vxr_fill_rect(x, y + 22, inner_w, 3, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 22, 24, 3, 0xFF0FE7FF);

    y += 42;
    vx_text::draw(x, y + 4, 11, "MEMORY", VxTheme::FG_SOFT, 0xFF171717);
    vx_text::draw(x + inner_w - 74, y + 4, 11, "116 MB / 4 GB", VxTheme::FG_SOFT, 0xFF171717);
    vxr_fill_rect(x, y + 22, inner_w, 3, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 22, 8, 3, 0xFF0FE7FF);

    y += 42;
    vx_text::draw(x, y + 4, 11, "DISK", VxTheme::FG_SOFT, 0xFF171717);
    vx_text::draw(x + inner_w - 74, y + 4, 11, "12.4 GB / 64 GB", VxTheme::FG_SOFT, 0xFF171717);
    vxr_fill_rect(x, y + 22, inner_w, 3, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 22, 34, 3, 0xFF0FE7FF);

    y += 42;
    vx_text::draw(x, y + 4, 11, "NETWORK", VxTheme::FG_SOFT, 0xFF171717);
    vxui_draw_rounded_rect(x + inner_w - 78, y - 2, 78, 18, 8, 0xFF2A2A2A);
    vx_text::draw(x + inner_w - 72, y + 9, 10, "vextryn-5G", VxTheme::FG_SOFT, 0xFF2A2A2A);
    vx_text::draw(x + 2, y + 30, 10, "1.8 Mb/s", VxTheme::MUTED, 0xFF171717);
    vx_text::draw(x + inner_w - 48, y + 30, 10, "0.4 Mb/s", VxTheme::MUTED, 0xFF171717);

    y += 52;
    vx_text::draw(x, y + 4, 11, "Brightness", VxTheme::FG_SOFT, 0xFF171717);
    vx_text::draw(x + inner_w - 24, y + 4, 11, "72%", VxTheme::FG_SOFT, 0xFF171717);
    vxr_fill_rect(x, y + 22, inner_w, 4, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 22, (inner_w * 72) / 100, 4, 0xFF0FE7FF);
    vxr_circle(x + (inner_w * 72) / 100, y + 24, 6, 0xFFF3F7FA);

    y += 42;
    vx_text::draw(x, y + 4, 11, "Volume", VxTheme::FG_SOFT, 0xFF171717);
    vx_text::draw(x + inner_w - 24, y + 4, 11, "61%", VxTheme::FG_SOFT, 0xFF171717);
    vxr_fill_rect(x, y + 22, inner_w, 4, 0x1FFFFFFF);
    vxr_fill_rect(x, y + 22, (inner_w * 61) / 100, 4, 0xFF0FE7FF);
    vxr_circle(x + (inner_w * 61) / 100, y + 24, 6, 0xFFF3F7FA);

    y += 46;
    vxr_fill_rect(x, y, inner_w, 1, 0x18FFFFFF);
    vxui_draw_rounded_rect(x, y + 10, 88, 24, 7, 0xFF202020);
    vxr_rounded_border(x, y + 10, 88, 24, 7, 0x40FFFFFF);
    vx_text::draw(x + 10, y + 26, 11, "End session...", VxTheme::FG, 0xFF202020);
}
