#ifndef APP_TERMINAL_HPP
#define APP_TERMINAL_HPP

static void draw_app_terminal(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t bg = 0xFF000000;
    uint32_t text_color = 0xFF00FF00;
    
    vxair_fb_fill_rect(w.x, w.y + 28, w.w, w.h - 28, bg);
    
    int cur_x = w.x + 10;
    int cur_y = w.y + 40;
    
    const char* prompt = "vxair@root:~$ ";
    for (int i=0; prompt[i]; i++) {
        draw_abstract_char(cur_x, cur_y, prompt[i], text_color);
        cur_x += 10;
    }
    
    for (int i=0; i<g_state.term_len; i++) {
        draw_abstract_char(cur_x, cur_y, g_state.term_buffer[i], text_color);
        cur_x += 10;
    }
    
    if (w.focused && (frame % 60 < 30)) {
        vxair_fb_fill_rect(cur_x, cur_y, 8, 14, text_color);
    }
    
    if (g_state.term_out_len > 0) {
        cur_y += 20;
        cur_x = w.x + 10;
        for (int i=0; i<g_state.term_out_len; i++) {
            draw_abstract_char(cur_x, cur_y, g_state.term_output[i], 0xFFAAAAAA);
            cur_x += 10;
        }
    }
}

#endif
