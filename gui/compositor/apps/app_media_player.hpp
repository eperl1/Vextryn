#pragma once
// V5 Media Player — VXUI-themed now-playing with album art and controls
#include <stdint.h>

static void draw_app_media_player(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    static bool is_playing = false;
    static uint64_t play_progress = 0;
    uint32_t accent = VxTheme::accent();

    int ax = w.x + 16;
    int ay = w.y + 48;
    int aw = w.w - 32;
    int ah = w.h - 64;
    if (aw <= 0 || ah <= 0) return;

    if (is_playing) play_progress++;

    // Album art area (left side)
    int art_size = (aw < ah) ? aw - 20 : ah - 60;
    if (art_size < 60) art_size = 60;
    int art_x = ax + (aw - art_size) / 2;
    int art_y = ay + 8;
    if (art_size > 120) art_size = 120;

    // Album art — animated gradient
    for (int y = 0; y < art_size; y++) {
        uint32_t c = lerp_color(VxTheme::accent_soft(), VxTheme::BASE_DARK, y, art_size);
        if (is_playing) {
            int wave = (y + play_progress / 4) % art_size;
            c = lerp_color(c, accent, wave, art_size);
        }
        vxair_fb_fill_rect(art_x, art_y + y, art_size, 1, c);
    }
    // Album art border
    vxair_fb_fill_rect(art_x - 1, art_y - 1, art_size + 2, 1, VxTheme::BORDER_BRIGHT);
    vxair_fb_fill_rect(art_x - 1, art_y + art_size, art_size + 2, 1, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(art_x - 1, art_y, 1, art_size, VxTheme::BORDER_STRONG);
    vxair_fb_fill_rect(art_x + art_size, art_y, 1, art_size, VxTheme::BORDER_STRONG);

    // Music note icon centered on art
    int ncx = art_x + art_size / 2;
    int ncy = art_y + art_size / 2;
    vxair_fb_fill_rect(ncx - 2, ncy - 16, 4, 20, VxTheme::TEXT_PRIMARY);
    vxair_fb_fill_rect(ncx + 2, ncy - 16, 12, 4, VxTheme::TEXT_PRIMARY);
    vxair_fb_fill_rect(ncx - 8, ncy + 2, 10, 8, VxTheme::TEXT_PRIMARY);

    // Track info
    int info_y = art_y + art_size + 16;
    const char* track = "Now Playing";
    for (int i = 0; track[i]; i++)
        draw_abstract_char(ax + aw / 2 - 55 + i * 10, info_y, track[i], VxTheme::TEXT_PRIMARY);
    const char* artist = "Vextryn Air";
    for (int i = 0; artist[i]; i++)
        draw_abstract_char(ax + aw / 2 - 44 + i * 8, info_y + 16, artist[i], VxTheme::TEXT_MUTED);

    // Progress bar
    int prog_y = info_y + 40;
    int prog_x = ax + 20;
    int prog_w = aw - 40;
    vxair_fb_fill_rect(prog_x, prog_y, prog_w, 6, VxTheme::BASE_DARK);
    vxair_fb_fill_rect(prog_x, prog_y, prog_w, 1, VxTheme::BORDER_SUBTLE);
    int fill_w = (play_progress % 1200) * prog_w / 1200;
    if (fill_w > 0) vxair_fb_fill_rect(prog_x, prog_y, fill_w, 6, accent);

    // Time labels
    const char* t1 = "0:42";
    for (int i = 0; t1[i]; i++) draw_abstract_char(prog_x + i * 8, prog_y + 12, t1[i], VxTheme::TEXT_MUTED);
    const char* t2 = "3:28";
    for (int i = 0; t2[i]; i++) draw_abstract_char(prog_x + prog_w - 32 + i * 8, prog_y + 12, t2[i], VxTheme::TEXT_MUTED);

    // Control buttons
    int ctrl_y = prog_y + 32;
    int play_x = ax + aw / 2 - 20;
    int play_y = ctrl_y;
    bool play_hover = (mouse_x >= play_x && mouse_x < play_x + 40 && mouse_y >= play_y && mouse_y < play_y + 40);

    // Play/pause button — accent circle
    for (int dy = -20; dy <= 20; dy++) {
        int half = 0;
        for (int dx = -20; dx <= 20; dx++) {
            if (dx * dx + dy * dy <= 400) half = dx;
        }
        if (half >= 0)
            vxair_fb_fill_rect(play_x + 20 - half, play_y + 20 + dy, half * 2 + 1, 1, play_hover ? VxTheme::accent_glow() : accent);
    }
    if (is_playing) {
        // Pause icon
        vxair_fb_fill_rect(play_x + 14, play_y + 12, 4, 16, VxTheme::TEXT_PRIMARY);
        vxair_fb_fill_rect(play_x + 22, play_y + 12, 4, 16, VxTheme::TEXT_PRIMARY);
    } else {
        // Play icon (triangle)
        for (int i = 0; i < 12; i++)
            vxair_fb_fill_rect(play_x + 14 + i, play_y + 10 + i, 2, 20 - i * 2, VxTheme::TEXT_PRIMARY);
    }

    if (clicked && play_hover) is_playing = !is_playing;

    // Prev/Next track buttons
    int prev_t_x = play_x - 56;
    int next_t_x = play_x + 56;
    bool pt_hover = (mouse_x >= prev_t_x && mouse_x < prev_t_x + 36 && mouse_y >= ctrl_y + 2 && mouse_y < ctrl_y + 36);
    bool nt_hover = (mouse_x >= next_t_x && mouse_x < next_t_x + 36 && mouse_y >= ctrl_y + 2 && mouse_y < ctrl_y + 36);

    // Prev track (|<)
    for (int i = 0; i < 8; i++) vxair_fb_fill_rect(prev_t_x + 10 + i, ctrl_y + 10 + i, 2, 16 - i * 2, pt_hover ? accent : VxTheme::TEXT_SECONDARY);
    vxair_fb_fill_rect(prev_t_x + 8, ctrl_y + 10, 2, 16, pt_hover ? accent : VxTheme::TEXT_SECONDARY);
    // Next track (>|)
    for (int i = 0; i < 8; i++) vxair_fb_fill_rect(next_t_x + 18 - i, ctrl_y + 10 + i, 2, 16 - i * 2, nt_hover ? accent : VxTheme::TEXT_SECONDARY);
    vxair_fb_fill_rect(next_t_x + 26, ctrl_y + 10, 2, 16, nt_hover ? accent : VxTheme::TEXT_SECONDARY);

    if (clicked && pt_hover) { play_progress = 0; }
    if (clicked && nt_hover) { play_progress += 120; }
}
