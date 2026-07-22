#ifndef APP_SNAKE_HPP
#define APP_SNAKE_HPP

static void draw_app_snake(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    uint32_t bg = 0xFF223322;
    vxair_fb_fill_rect(w.x, w.y + 28, w.w, w.h - 28, bg);
    
    if (w.focused && !g_state.snake_dead && frame - g_state.last_snake_move > 15) {
        g_state.last_snake_move = frame;
        
        for (int i=g_state.snake_len-1; i>0; i--) {
            g_state.snake_x[i] = g_state.snake_x[i-1];
            g_state.snake_y[i] = g_state.snake_y[i-1];
        }
        
        if (g_state.snake_dir == 0) g_state.snake_y[0]--;
        else if (g_state.snake_dir == 1) g_state.snake_y[0]++;
        else if (g_state.snake_dir == 2) g_state.snake_x[0]--;
        else if (g_state.snake_dir == 3) g_state.snake_x[0]++;
        
        if (g_state.snake_x[0] < 0) g_state.snake_x[0] = 39;
        if (g_state.snake_x[0] >= 40) g_state.snake_x[0] = 0;
        if (g_state.snake_y[0] < 0) g_state.snake_y[0] = 39;
        if (g_state.snake_y[0] >= 40) g_state.snake_y[0] = 0;
        
        if (g_state.snake_x[0] == g_state.food_x && g_state.snake_y[0] == g_state.food_y) {
            if (g_state.snake_len < 99) g_state.snake_len++;
            g_state.food_x = (frame * 17) % 40;
            g_state.food_y = (frame * 13) % 40;
        }
        
        for (int i=1; i<g_state.snake_len; i++) {
            if (g_state.snake_x[0] == g_state.snake_x[i] && g_state.snake_y[0] == g_state.snake_y[i]) {
                g_state.snake_dead = true;
            }
        }
    }
    
    // Draw food
    vxair_fb_fill_rect(w.x + g_state.food_x * 10, w.y + 28 + g_state.food_y * 10, 10, 10, 0xFFFF0000);
    
    // Draw snake
    for (int i=0; i<g_state.snake_len; i++) {
        uint32_t c = (i == 0) ? 0xFF00FF00 : 0xFF009900;
        vxair_fb_fill_rect(w.x + g_state.snake_x[i] * 10, w.y + 28 + g_state.snake_y[i] * 10, 10, 10, c);
    }
    
    if (g_state.snake_dead) {
        const char* msg = "GAME OVER";
        int tx = w.x + w.w/2 - 45;
        int ty = w.y + w.h/2;
        for (int i=0; msg[i]; i++) {
            draw_abstract_char(tx, ty, msg[i], 0xFFFFFFFF);
            tx += 10;
        }
        
        if (clicked && mouse_x > w.x && mouse_x < w.x + w.w && mouse_y > w.y && mouse_y < w.y + w.h) {
            g_state.snake_len = 3;
            g_state.snake_x[0] = 10; g_state.snake_y[0] = 10;
            g_state.snake_x[1] = 9; g_state.snake_y[1] = 10;
            g_state.snake_x[2] = 8; g_state.snake_y[2] = 10;
            g_state.snake_dir = 3;
            g_state.food_x = 15; g_state.food_y = 15;
            g_state.snake_dead = false;
        }
    }
}

#endif
