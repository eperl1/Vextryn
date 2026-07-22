#pragma once

#include <stdint.h>

void draw_app_media_player(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    static bool is_playing = false;
    static uint64_t play_progress = 0;

    int usable_x = w.x + 2;
    int usable_y = w.y + 28;
    int usable_w = w.w - 4;
    int usable_h = w.h - 30;

    if (usable_w <= 0 || usable_h <= 0) return;

    if (is_playing) {
        play_progress++;
    }

    int control_bar_h = 40;
    if (usable_h < control_bar_h) {
        control_bar_h = usable_h;
    }

    int vid_h = usable_h - control_bar_h;
    if (vid_h > 0) {
        // Draw video area (Black)
        vxair_fb_fill_rect(usable_x, usable_y, usable_w, vid_h, 0xFF000000);

        // Draw some basic "video content" to make it look alive when playing
        if (is_playing && usable_w > 40 && vid_h > 40) {
            int bounce_w = usable_w / 4;
            int bounce_h = vid_h / 4;
            if (bounce_w > 0 && bounce_h > 0) {
                int bounce_area_w = usable_w - bounce_w;
                int bounce_area_h = vid_h - bounce_h;
                if (bounce_area_w > 0 && bounce_area_h > 0) {
                    // Bouncing rectangle animation
                    int px = (play_progress / 2) % (bounce_area_w * 2);
                    if (px > bounce_area_w) px = (bounce_area_w * 2) - px;
                    
                    int py = (play_progress / 3) % (bounce_area_h * 2);
                    if (py > bounce_area_h) py = (bounce_area_h * 2) - py;
                    
                    vxair_fb_fill_rect(usable_x + px, usable_y + py, bounce_w, bounce_h, 0xFF333333);
                }
            }
        }
    }

    // Draw control bar background
    int cb_y = usable_y + vid_h;
    vxair_fb_fill_rect(usable_x, cb_y, usable_w, control_bar_h, 0xFF222222);

    int btn_w = 30;
    int btn_h = 30;
    int btn_x = usable_x + 5;
    int btn_y = cb_y + 5;

    // Handle click for play/pause button
    if (clicked) {
        if (mouse_x >= btn_x && mouse_x <= btn_x + btn_w &&
            mouse_y >= btn_y && mouse_y <= btn_y + btn_h) {
            is_playing = !is_playing;
        }
    }

    // Draw button background
    vxair_fb_fill_rect(btn_x, btn_y, btn_w, btn_h, 0xFF444444);

    if (is_playing) {
        // Pause icon (two vertical bars)
        vxair_fb_fill_rect(btn_x + 8, btn_y + 8, 5, 14, 0xFFFFFFFF);
        vxair_fb_fill_rect(btn_x + 17, btn_y + 8, 5, 14, 0xFFFFFFFF);
    } else {
        // Play icon (triangle)
        for (int i = 0; i < 8; ++i) {
            vxair_fb_fill_rect(btn_x + 10 + i, btn_y + 7 + i, 1, 16 - i * 2, 0xFFFFFFFF);
        }
    }

    // Draw progress bar
    int prog_x = btn_x + btn_w + 10;
    int prog_y = cb_y + 15;
    int prog_w = usable_w - (btn_w + 20); // 5 margin left + 30 btn + 10 gap + 5 margin right = 50 total subtracted from usable_w
    int prog_h = 10;

    if (prog_w > 0) {
        // Progress bar background
        vxair_fb_fill_rect(prog_x, prog_y, prog_w, prog_h, 0xFF555555);
        
        // Progress bar filled part
        int fill_w = (play_progress % 1000) * prog_w / 1000;
        vxair_fb_fill_rect(prog_x, prog_y, fill_w, prog_h, 0xFFFF0000); // Red
    }
}
