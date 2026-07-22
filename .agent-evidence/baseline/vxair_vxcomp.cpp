extern "C" {
#include "../../drivers/gpu/vxair_gop.h"
    extern void vxair_hpet_sleep_ms(uint32_t ms);
    extern void vxair_log_info(const char* fmt, ...);

    enum VxAppId {
        VX_APP_NONE = 0,
        VX_APP_CALCULATOR,
        VX_APP_NOTES,
        VX_APP_SYSMON,
        VX_APP_TERMINAL,
        VX_APP_FILE_MANAGER,
        VX_APP_SETTINGS,
        VX_APP_BROWSER,
        VX_APP_MEDIA_PLAYER,
        VX_APP_IMAGE_VIEWER,
        VX_APP_CALENDAR
    };

    struct VxWindow {
        bool open;
        VxAppId app;
        int x;
        int y;
        int w;
        int h;
        bool dragging;
        int drag_offset_x;
        int drag_offset_y;
        bool focused;
    };

    struct VxGuiState {
        bool launcher_open;
        bool previous_left_down;
        int mouse_x;
        int mouse_y;
        int focused_window;
        VxWindow windows[10];

        char notes[1024];
        int notes_length;
        bool shift_down;

        int calc_accumulator;
        int calc_pending_value;
        char calc_operator;
        bool calc_replace_display;
        bool calc_error;
    };

    static VxGuiState g_state;
    static uint64_t g_frame = 0;
    static int g_z_order[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Forward declarations
    static uint32_t lerp_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t);
    static void draw_polished_desktop(uint32_t W, uint32_t H);
    static void draw_window(VxWindow& w, bool clicked);
    static void draw_digit(int x, int y, int digit, uint32_t color);
    static void draw_number(int x, int y, int num, uint32_t color);
    static void draw_segment(int x, int y, int length, bool horizontal, uint32_t color);
    static void draw_abstract_char(int x, int y, char c, uint32_t color);

    // Include parallel agent apps
    #include "apps/app_terminal.hpp"
    #include "apps/app_file_manager.hpp"
    #include "apps/app_settings.hpp"
    #include "apps/app_browser.hpp"
    #include "apps/app_media_player.hpp"
    #include "apps/app_image_viewer.hpp"
    #include "apps/app_calendar.hpp"

    static inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
        return ret;
    }
    static inline void outb(uint16_t port, uint8_t val) {
        __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    }

    static void mouse_wait(uint8_t type) {
        uint32_t timeout = 100000;
        if (type == 0) { while (timeout--) { if ((inb(0x64) & 1) == 1) return; } }
        else { while (timeout--) { if ((inb(0x64) & 2) == 0) return; } }
    }

    static void mouse_write(uint8_t data) {
        mouse_wait(1);
        outb(0x64, 0xD4);
        mouse_wait(1);
        outb(0x60, data);
        mouse_wait(0);
        inb(0x60);
    }

    static void mouse_init(void) {
        mouse_wait(1);
        outb(0x64, 0xA8);
        mouse_wait(1);
        outb(0x64, 0x20);
        mouse_wait(0);
        uint8_t status = inb(0x60) | 2;
        mouse_wait(1);
        outb(0x64, 0x60);
        mouse_wait(1);
        outb(0x60, status);
        mouse_write(0xF6);
        mouse_write(0xF4);
    }

    static char scancode_to_ascii(uint8_t scancode, bool shift) {
        if (scancode == 0x1E) return shift ? 'A' : 'a';
        if (scancode == 0x30) return shift ? 'B' : 'b';
        if (scancode == 0x2E) return shift ? 'C' : 'c';
        if (scancode == 0x20) return shift ? 'D' : 'd';
        if (scancode == 0x12) return shift ? 'E' : 'e';
        if (scancode == 0x21) return shift ? 'F' : 'f';
        if (scancode == 0x22) return shift ? 'G' : 'g';
        if (scancode == 0x23) return shift ? 'H' : 'h';
        if (scancode == 0x17) return shift ? 'I' : 'i';
        if (scancode == 0x24) return shift ? 'J' : 'j';
        if (scancode == 0x25) return shift ? 'K' : 'k';
        if (scancode == 0x26) return shift ? 'L' : 'l';
        if (scancode == 0x32) return shift ? 'M' : 'm';
        if (scancode == 0x31) return shift ? 'N' : 'n';
        if (scancode == 0x18) return shift ? 'O' : 'o';
        if (scancode == 0x19) return shift ? 'P' : 'p';
        if (scancode == 0x10) return shift ? 'Q' : 'q';
        if (scancode == 0x13) return shift ? 'R' : 'r';
        if (scancode == 0x1F) return shift ? 'S' : 's';
        if (scancode == 0x14) return shift ? 'T' : 't';
        if (scancode == 0x16) return shift ? 'U' : 'u';
        if (scancode == 0x2F) return shift ? 'V' : 'v';
        if (scancode == 0x11) return shift ? 'W' : 'w';
        if (scancode == 0x2D) return shift ? 'X' : 'x';
        if (scancode == 0x15) return shift ? 'Y' : 'y';
        if (scancode == 0x2C) return shift ? 'Z' : 'z';
        if (scancode == 0x02) return shift ? '!' : '1';
        if (scancode == 0x03) return shift ? '@' : '2';
        if (scancode == 0x04) return shift ? '#' : '3';
        if (scancode == 0x05) return shift ? '$' : '4';
        if (scancode == 0x06) return shift ? '%' : '5';
        if (scancode == 0x07) return shift ? '^' : '6';
        if (scancode == 0x08) return shift ? '&' : '7';
        if (scancode == 0x09) return shift ? '*' : '8';
        if (scancode == 0x0A) return shift ? '(' : '9';
        if (scancode == 0x0B) return shift ? ')' : '0';
        if (scancode == 0x39) return ' ';
        if (scancode == 0x1C) return '\n';
        if (scancode == 0x0E) return '\b';
        if (scancode == 0x01) return 27; // Esc
        return 0;
    }

    static void bring_to_front(int window_idx) {
        g_state.focused_window = window_idx;
        for (int i = 0; i < 10; i++) {
            g_state.windows[i].focused = (i == window_idx);
        }
        int pos = -1;
        for (int i = 0; i < 10; i++) {
            if (g_z_order[i] == window_idx) {
                pos = i;
                break;
            }
        }
        if (pos != -1) {
            for (int i = pos; i < 9; i++) {
                g_z_order[i] = g_z_order[i+1];
            }
            g_z_order[9] = window_idx;
        }
    }

    static void open_app(VxAppId app_id) {
        for (int i = 0; i < 10; i++) {
            if (g_state.windows[i].app == app_id) {
                g_state.windows[i].open = true;
                bring_to_front(i);
                break;
            }
        }
    }

    static bool g_window_clicked[10] = {false};

    static void handle_input(uint32_t W, uint32_t H) {
        static uint8_t mbyte[3];
        static int cycle = 0;
        
        for (int i=0; i<10; i++) g_window_clicked[i] = false;

        while ((inb(0x64) & 1) != 0) {
            uint8_t status = inb(0x64);
            uint8_t data = inb(0x60);
            
            if (status & 0x20) {
                // Mouse
                mbyte[cycle++] = data;
                if (cycle == 3) {
                    cycle = 0;
                    uint8_t state = mbyte[0];
                    bool left_down = (state & 0x01) != 0;
                    
                    int32_t rdx = mbyte[1]; if (state & 0x10) rdx -= 256;
                    int32_t rdy = mbyte[2]; if (state & 0x20) rdy -= 256;
                    
                    g_state.mouse_x += rdx / 4;
                    g_state.mouse_y -= rdy / 4;
                    if (g_state.mouse_x < 0) g_state.mouse_x = 0;
                    if (g_state.mouse_y < 0) g_state.mouse_y = 0;
                    if (g_state.mouse_x > (int)W - 1) g_state.mouse_x = W - 1;
                    if (g_state.mouse_y > (int)H - 1) g_state.mouse_y = H - 1;

                    bool clicked = (left_down && !g_state.previous_left_down);
                    bool released = (!left_down && g_state.previous_left_down);
                    g_state.previous_left_down = left_down;

                    // Handle window dragging
                    if (left_down) {
                        for (int i=0; i<10; i++) {
                            if (g_state.windows[i].dragging) {
                                g_state.windows[i].x = g_state.mouse_x - g_state.windows[i].drag_offset_x;
                                g_state.windows[i].y = g_state.mouse_y - g_state.windows[i].drag_offset_y;
                                // Clamp
                                if (g_state.windows[i].y < 0) g_state.windows[i].y = 0;
                                if (g_state.windows[i].y > (int)H - 80) g_state.windows[i].y = H - 80;
                                if (g_state.windows[i].x < -g_state.windows[i].w + 40) g_state.windows[i].x = -g_state.windows[i].w + 40;
                                if (g_state.windows[i].x > (int)W - 40) g_state.windows[i].x = W - 40;
                                break;
                            }
                        }
                    } else if (released) {
                        for (int i=0; i<10; i++) g_state.windows[i].dragging = false;
                    }

                    if (clicked) {
                        bool handled = false;
                        uint32_t tb_y = H - 56;
                        uint32_t mx = g_state.mouse_x;
                        uint32_t my = g_state.mouse_y;

                        // 1. Launcher button
                        if (mx >= 12 && mx <= 52 && my >= tb_y + 8 && my <= tb_y + 48) {
                            g_state.launcher_open = !g_state.launcher_open;
                            handled = true;
                        } 
                        // 2. Launcher open
                        else if (g_state.launcher_open) {
                            uint32_t menu_w = 460, menu_h = 260, menu_x = 12, menu_y = H - 56 - menu_h - 8;
                            if (mx >= menu_x && mx <= menu_x + menu_w && my >= menu_y && my <= menu_y + menu_h) {
                                // 10 Apps layout: 2 columns of 5
                                for (int i=0; i<10; i++) {
                                    int col = i / 5;
                                    int row = i % 5;
                                    uint32_t item_x = menu_x + 8 + col * 220;
                                    uint32_t item_y = menu_y + 16 + row * 44;
                                    if (mx >= item_x && mx <= item_x + 210 && my >= item_y && my <= item_y + 36) {
                                        open_app((VxAppId)(i + 1));
                                        g_state.launcher_open = false;
                                    }
                                }
                                handled = true;
                            } else {
                                g_state.launcher_open = false;
                            }
                        }

                        // 3. Windows (top to bottom)
                        if (!handled) {
                            for (int z = 9; z >= 0; z--) {
                                int i = g_z_order[z];
                                VxWindow& w = g_state.windows[i];
                                if (!w.open) continue;
                                
                                if (mx >= (uint32_t)w.x && mx <= (uint32_t)w.x + w.w && 
                                    my >= (uint32_t)w.y && my <= (uint32_t)w.y + w.h) {
                                    
                                    bring_to_front(i);
                                    handled = true;

                                    // Close button
                                    if (mx >= (uint32_t)w.x + w.w - 24 && mx <= (uint32_t)w.x + w.w - 4 && 
                                        my >= (uint32_t)w.y + 4 && my <= (uint32_t)w.y + 24) {
                                        w.open = false;
                                        break;
                                    }
                                    
                                    // Title bar drag
                                    if (my >= (uint32_t)w.y && my <= (uint32_t)w.y + 28) {
                                        w.dragging = true;
                                        w.drag_offset_x = mx - w.x;
                                        w.drag_offset_y = my - w.y;
                                        break;
                                    }

                                    // Inside app area click
                                    g_window_clicked[i] = true;
                                    break;
                                }
                            }
                            if (!handled) {
                                g_state.focused_window = -1; // Clicked desktop
                                for (int i=0; i<10; i++) g_state.windows[i].focused = false;
                            }
                        }
                    }
                }
            } else {
                // Keyboard
                if (data == 0x2A || data == 0x36) g_state.shift_down = true;
                else if (data == 0xAA || data == 0xB6) g_state.shift_down = false;
                else if ((data & 0x80) == 0) {
                    char c = scancode_to_ascii(data, g_state.shift_down);
                    if (c == 27) {
                        g_state.launcher_open = false;
                    } else if (c != 0) {
                        if (g_state.focused_window != -1 && g_state.windows[g_state.focused_window].app == VX_APP_NOTES) {
                            if (c == '\b') {
                                if (g_state.notes_length > 0) g_state.notes_length--;
                            } else if (g_state.notes_length < 1023) {
                                g_state.notes[g_state.notes_length++] = c;
                            }
                        }
                    }
                }
            }
        }
    }

    static void draw_segment(int x, int y, int length, bool horizontal, uint32_t color) {
        if (horizontal) vxair_fb_fill_rect(x, y, length, 3, color);
        else vxair_fb_fill_rect(x, y, 3, length, color);
    }

    static void draw_digit(int x, int y, int digit, uint32_t color) {
        bool segs[10][7] = {
            {1,1,1,1,1,1,0}, // 0
            {0,1,1,0,0,0,0}, // 1
            {1,1,0,1,1,0,1}, // 2
            {1,1,1,1,0,0,1}, // 3
            {0,1,1,0,0,1,1}, // 4
            {1,0,1,1,0,1,1}, // 5
            {1,0,1,1,1,1,1}, // 6
            {1,1,1,0,0,0,0}, // 7
            {1,1,1,1,1,1,1}, // 8
            {1,1,1,1,0,1,1}  // 9
        };
        int d = digit;
        if (d < 0) d = 0; if (d > 9) d = 9;
        if (segs[d][0]) draw_segment(x, y, 12, true, color);
        if (segs[d][1]) draw_segment(x+11, y, 12, false, color);
        if (segs[d][2]) draw_segment(x+11, y+11, 12, false, color);
        if (segs[d][3]) draw_segment(x, y+22, 14, true, color);
        if (segs[d][4]) draw_segment(x, y+11, 12, false, color);
        if (segs[d][5]) draw_segment(x, y, 12, false, color);
        if (segs[d][6]) draw_segment(x, y+11, 12, true, color);
    }

    static void draw_number(int x, int y, int num, uint32_t color) {
        if (num < 0) {
            draw_segment(x, y+11, 8, true, color); // minus
            x += 16;
            num = -num;
        }
        if (num == 0) {
            draw_digit(x, y, 0, color);
            return;
        }
        int digits[12];
        int count = 0;
        while (num > 0 && count < 12) {
            digits[count++] = num % 10;
            num /= 10;
        }
        for (int i = count - 1; i >= 0; i--) {
            draw_digit(x, y, digits[i], color);
            x += 20;
        }
    }

    static void draw_abstract_char(int x, int y, char c, uint32_t color) {
        if (c == ' ') return;
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 5; j++) {
                int bit = ((c * (i + 13) * (j + 17)) ^ (c * 19)) % 2;
                if (bit) vxair_fb_fill_rect(x + j*2, y + i*2, 2, 2, color);
            }
        }
    }

    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover) {
        vxair_fb_fill_rect(x, y, 32, 32, hover ? 0xFF1E293B : 0xFF0F172A);
        vxair_fb_fill_rect(x+1, y+1, 30, 30, hover ? 0xFF334155 : 0xFF1E293B);

        uint32_t cyan = 0xFF00F0FF;
        uint32_t pink = 0xFFFF003C;
        uint32_t green = 0xFF39FF14;
        uint32_t yellow = 0xFFFDE047;
        uint32_t white = 0xFFFFFFFF;

        if (app_index == 0) {
            vxair_fb_fill_rect(x+6, y+6, 20, 20, 0xFF0F172A);
            vxair_fb_fill_rect(x+8, y+8, 16, 6, cyan);
            for(int r=0;r<2;r++) for(int c=0;c<3;c++) vxair_fb_fill_rect(x+8+c*6, y+16+r*6, 4, 4, white);
        } else if (app_index == 1) {
            vxair_fb_fill_rect(x+8, y+4, 16, 24, white);
            vxair_fb_fill_rect(x+10, y+8, 12, 2, yellow);
            vxair_fb_fill_rect(x+10, y+12, 12, 2, cyan);
            vxair_fb_fill_rect(x+10, y+16, 8, 2, cyan);
        } else if (app_index == 2) {
            vxair_fb_fill_rect(x+4, y+16, 24, 2, 0xFF334155);
            vxair_fb_fill_rect(x+6, y+16, 4, 2, green);
            vxair_fb_fill_rect(x+10, y+8, 4, 10, green);
            vxair_fb_fill_rect(x+14, y+20, 4, 6, green);
            vxair_fb_fill_rect(x+18, y+16, 8, 2, green);
        } else if (app_index == 3) {
            vxair_fb_fill_rect(x+4, y+6, 24, 20, 0xFF000000);
            vxair_fb_fill_rect(x+6, y+10, 4, 2, green);
            vxair_fb_fill_rect(x+8, y+12, 4, 2, green);
            vxair_fb_fill_rect(x+6, y+14, 4, 2, green);
            if ((g_frame/20)%2 == 0) vxair_fb_fill_rect(x+14, y+14, 6, 2, green);
        } else if (app_index == 4) {
            vxair_fb_fill_rect(x+4, y+8, 10, 4, yellow);
            vxair_fb_fill_rect(x+4, y+12, 24, 14, yellow);
            vxair_fb_fill_rect(x+6, y+14, 20, 10, 0xFFFEF08A);
        } else if (app_index == 5) {
            vxair_fb_fill_rect(x+12, y+12, 8, 8, 0xFF94A3B8);
            vxair_fb_fill_rect(x+14, y+14, 4, 4, 0xFF0F172A);
            vxair_fb_fill_rect(x+14, y+8, 4, 4, 0xFF94A3B8);
            vxair_fb_fill_rect(x+14, y+20, 4, 4, 0xFF94A3B8);
            vxair_fb_fill_rect(x+8, y+14, 4, 4, 0xFF94A3B8);
            vxair_fb_fill_rect(x+20, y+14, 4, 4, 0xFF94A3B8);
        } else if (app_index == 6) {
            vxair_fb_fill_rect(x+4, y+4, 24, 24, cyan);
            vxair_fb_fill_rect(x+6, y+6, 20, 20, 0xFF0F172A);
            vxair_fb_fill_rect(x+6, y+14, 20, 4, cyan);
            vxair_fb_fill_rect(x+14, y+6, 4, 20, cyan);
        } else if (app_index == 7) {
            vxair_fb_fill_rect(x+4, y+6, 24, 20, 0xFF1E293B);
            vxair_fb_fill_rect(x+12, y+10, 2, 12, pink);
            vxair_fb_fill_rect(x+14, y+12, 2, 8, pink);
            vxair_fb_fill_rect(x+16, y+14, 2, 4, pink);
        } else if (app_index == 8) {
            vxair_fb_fill_rect(x+4, y+6, 24, 20, white);
            vxair_fb_fill_rect(x+6, y+8, 20, 16, 0xFF1E293B);
            vxair_fb_fill_rect(x+20, y+10, 4, 4, yellow);
            vxair_fb_fill_rect(x+6, y+18, 10, 6, cyan);
            vxair_fb_fill_rect(x+12, y+14, 10, 10, pink);
        } else if (app_index == 9) {
            vxair_fb_fill_rect(x+6, y+4, 20, 24, white);
            vxair_fb_fill_rect(x+6, y+4, 20, 6, pink);
            for(int r=0;r<3;r++) for(int c=0;c<3;c++) vxair_fb_fill_rect(x+8+c*5, y+12+r*5, 3, 3, 0xFF94A3B8);
        }
    }

    static void draw_window(VxWindow& w, bool clicked) {
        vxair_fb_fill_rect(w.x - 2, w.y - 2, w.w + 4, w.h + 4, w.focused ? 0xFF00F0FF : 0xFF334155);
        vxair_fb_fill_rect(w.x - 1, w.y - 1, w.w + 2, w.h + 2, 0xFF0F172A);
        vxair_fb_fill_rect(w.x, w.y, w.w, w.h, 0xFF1E293B); // Fill
        
        vxair_fb_fill_rect(w.x, w.y, w.w, 28, w.focused ? 0xFF00F0FF : 0xFF334155);
        vxair_fb_fill_rect(w.x + 2, w.y + 2, w.w - 4, 24, 0xFF0F172A);
        
        bool close_hover = (g_state.mouse_x >= w.x + w.w - 24 && g_state.mouse_x <= w.x + w.w - 4 && 
                            g_state.mouse_y >= w.y + 4 && g_state.mouse_y <= w.y + 24);
        vxair_fb_fill_rect(w.x + w.w - 24, w.y + 4, 20, 20, close_hover ? 0xFFFF003C : 0xFF990024);

        if (w.app == VX_APP_CALCULATOR) {
            vxair_fb_fill_rect(w.x + 20, w.y + 40, w.w - 40, 50, 0xFFE8F0F4); // Display
            
            if (g_state.calc_error) {
                vxair_fb_fill_rect(w.x + 30, w.y + 55, 15, 20, 0xFFE05050);
            } else {
                draw_number(w.x + 30, w.y + 55, g_state.calc_accumulator, 0xFF304050);
            }

            uint32_t bx = w.x + 20;
            uint32_t by = w.y + 110;
            uint32_t bw = (w.w - 50) / 4;
            uint32_t bh = 60;
            for (int r=0; r<4; r++) {
                for (int c=0; c<4; c++) {
                    uint32_t cx = bx + c*(bw + 10);
                    uint32_t cy = by + r*(bh + 10);
                    bool hover = (g_state.mouse_x >= (int)cx && g_state.mouse_x <= (int)(cx + bw) && 
                                  g_state.mouse_y >= (int)cy && g_state.mouse_y <= (int)(cy + bh));
                    vxair_fb_fill_rect(cx, cy, bw, bh, hover ? 0xFFEAF1F5 : 0xFFF0F5F8);
                    
                    char key = 0;
                    if (r==0) { if(c<3) key='7'+c; else key='/'; }
                    else if (r==1) { if(c<3) key='4'+c; else key='*'; }
                    else if (r==2) { if(c<3) key='1'+c; else key='-'; }
                    else if (r==3) { if(c==0) key='C'; else if(c==1) key='0'; else if(c==2) key='='; else key='+'; }
                    
                    if (clicked && hover) {
                        if (key >= '0' && key <= '9') {
                            if (g_state.calc_replace_display) {
                                g_state.calc_accumulator = key - '0';
                                g_state.calc_replace_display = false;
                                g_state.calc_error = false;
                            } else {
                                g_state.calc_accumulator = g_state.calc_accumulator * 10 + (key - '0');
                            }
                        } else if (key == 'C') {
                            g_state.calc_accumulator = 0;
                            g_state.calc_pending_value = 0;
                            g_state.calc_operator = 0;
                            g_state.calc_error = false;
                        } else if (key == '+' || key == '-' || key == '*' || key == '/') {
                            if (g_state.calc_operator) {
                                if (g_state.calc_operator == '+') g_state.calc_pending_value += g_state.calc_accumulator;
                                else if (g_state.calc_operator == '-') g_state.calc_pending_value -= g_state.calc_accumulator;
                                else if (g_state.calc_operator == '*') g_state.calc_pending_value *= g_state.calc_accumulator;
                                else if (g_state.calc_operator == '/') {
                                    if (g_state.calc_accumulator == 0) g_state.calc_error = true;
                                    else g_state.calc_pending_value /= g_state.calc_accumulator;
                                }
                            } else {
                                g_state.calc_pending_value = g_state.calc_accumulator;
                            }
                            g_state.calc_accumulator = 0;
                            g_state.calc_operator = key;
                            g_state.calc_replace_display = true;
                        } else if (key == '=') {
                            if (g_state.calc_operator == '+') g_state.calc_accumulator = g_state.calc_pending_value + g_state.calc_accumulator;
                            else if (g_state.calc_operator == '-') g_state.calc_accumulator = g_state.calc_pending_value - g_state.calc_accumulator;
                            else if (g_state.calc_operator == '*') g_state.calc_accumulator = g_state.calc_pending_value * g_state.calc_accumulator;
                            else if (g_state.calc_operator == '/') {
                                if (g_state.calc_accumulator == 0) g_state.calc_error = true;
                                else g_state.calc_accumulator = g_state.calc_pending_value / g_state.calc_accumulator;
                            }
                            g_state.calc_pending_value = 0;
                            g_state.calc_operator = 0;
                            g_state.calc_replace_display = true;
                        }
                    }

                    if (key >= '0' && key <= '9') {
                        draw_digit(cx + bw/2 - 6, cy + bh/2 - 11, key - '0', 0xFF607080);
                    } else if (key == '-') {
                        draw_segment(cx + bw/2 - 6, cy + bh/2 - 1, 12, true, 0xFF607080);
                    } else if (key == '=') {
                        draw_segment(cx + bw/2 - 6, cy + bh/2 - 4, 12, true, 0xFF607080);
                        draw_segment(cx + bw/2 - 6, cy + bh/2 + 2, 12, true, 0xFF607080);
                    } else if (key == '+') {
                        draw_segment(cx + bw/2 - 6, cy + bh/2 - 1, 12, true, 0xFF607080);
                        draw_segment(cx + bw/2 - 1, cy + bh/2 - 6, 10, false, 0xFF607080);
                    } else {
                        // Just a placeholder dot for ops
                        vxair_fb_fill_rect(cx + bw/2 - 2, cy + bh/2 - 2, 4, 4, 0xFF607080);
                    }
                }
            }
        } else if (w.app == VX_APP_NOTES) {
            vxair_fb_fill_rect(w.x + 30, w.y + 28, 2, w.h - 30, 0xFFFFD0D0); // Margin
            for (int i=0; i<13; i++) {
                vxair_fb_fill_rect(w.x + 2, w.y + 60 + i*28, w.w - 4, 1, 0xFFE0E8ED);
            }
            int cur_x = w.x + 40;
            int cur_y = w.y + 40;
            for (int i=0; i<g_state.notes_length; i++) {
                if (g_state.notes[i] == '\n') {
                    cur_x = w.x + 40;
                    cur_y += 28;
                } else {
                    draw_abstract_char(cur_x, cur_y, g_state.notes[i], 0xFF405060);
                    cur_x += 12;
                    if (cur_x > w.x + w.w - 20) {
                        cur_x = w.x + 40;
                        cur_y += 28;
                    }
                }
            }
            if (w.focused && (g_frame % 60 < 30)) {
                vxair_fb_fill_rect(cur_x, cur_y, 2, 14, 0xFF405060);
            }
        } else if (w.app == VX_APP_SYSMON) {
            uint32_t cpu = 20 + (g_frame % 40);
            uint32_t mem = 50 + (g_frame % 20);
            
            vxair_fb_fill_rect(w.x + 20, w.y + 50, 400, 20, 0xFFE0E0E0);
            vxair_fb_fill_rect(w.x + 20, w.y + 50, cpu * 4, 20, 0xFF4A9EFF);
            vxair_fb_fill_rect(w.x + 20, w.y + 90, 400, 20, 0xFFE0E0E0);
            vxair_fb_fill_rect(w.x + 20, w.y + 90, mem * 4, 20, 0xFF4A9EFF);

            for(int i=0; i<48; i++) {
                int h = 10 + ((i*7 + g_frame) % 40);
                vxair_fb_fill_rect(w.x + 20 + i*8, w.y + 190 - h, 6, h, 0xFF8CCBE8);
            }
            
            for(int i=0; i<4; i++) {
                vxair_fb_fill_rect(w.x + 20, w.y + 220 + i*24, 16, 16, 0xFFB0C4DE);
                vxair_fb_fill_rect(w.x + 44, w.y + 226, 100 + (i*20), 4, 0xFFD0DFE8);
            }
        } else if (w.app == VX_APP_TERMINAL) {
            draw_app_terminal(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_FILE_MANAGER) {
            draw_app_file_manager(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_SETTINGS) {
            draw_app_settings(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_BROWSER) {
            draw_app_browser(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_MEDIA_PLAYER) {
            draw_app_media_player(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_IMAGE_VIEWER) {
            draw_app_image_viewer(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_CALENDAR) {
            draw_app_calendar(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        }
    }

    static uint32_t lerp_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t) {
        if (max_t == 0) return c1;
        uint32_t r1 = (c1 >> 16) & 0xFF;
        uint32_t g1 = (c1 >> 8) & 0xFF;
        uint32_t b1 = c1 & 0xFF;
        uint32_t r2 = (c2 >> 16) & 0xFF;
        uint32_t g2 = (c2 >> 8) & 0xFF;
        uint32_t b2 = c2 & 0xFF;
        uint32_t r = r1 + (r2 - r1) * t / max_t;
        uint32_t g = g1 + (g2 - g1) * t / max_t;
        uint32_t b = b1 + (b2 - b1) * t / max_t;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    static void draw_polished_desktop(uint32_t W, uint32_t H) {
        for (uint32_t y = 0; y < H; y++) {
            uint32_t color = lerp_color(0xFF020617, 0xFF0F172A, y, H);
            vxair_fb_fill_rect(0, y, W, 1, color);
        }
        
        for (uint32_t y = 0; y < H; y += 40) vxair_fb_fill_rect(0, y, W, 1, 0xFF1E293B);
        for (uint32_t x = 0; x < W; x += 40) vxair_fb_fill_rect(x, 0, 1, H, 0xFF1E293B);

        uint32_t tb_y = H - 56;
        vxair_fb_fill_rect(0, tb_y - 2, W, 2, 0xFF00F0FF);
        vxair_fb_fill_rect(0, tb_y, W, 56, 0xFF020617);

        uint32_t lx = 12, ly = tb_y + 8;
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
        
        vxair_fb_fill_rect(lx, ly, 40, 40, launcher_hover ? 0xFF00F0FF : 0xFF1E293B);
        vxair_fb_fill_rect(lx + 1, ly + 1, 38, 38, g_state.launcher_open ? 0xFF0F172A : 0xFF020617);

        vxair_fb_fill_rect(lx + 11, ly + 11, 6, 6, 0xFF00F0FF);
        vxair_fb_fill_rect(lx + 23, ly + 11, 6, 6, 0xFF00F0FF);
        vxair_fb_fill_rect(lx + 11, ly + 23, 6, 6, 0xFF00F0FF);
        vxair_fb_fill_rect(lx + 23, ly + 23, 6, 6, 0xFF00F0FF);

        uint32_t tx_base = W - 220;
        uint32_t ty = tb_y + 14;
        vxair_fb_fill_rect(tx_base, ty, 28, 28, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 6, ty + 16, 4, 6, 0xFF00F0FF);
        vxair_fb_fill_rect(tx_base + 12, ty + 12, 4, 10, 0xFF00F0FF);
        vxair_fb_fill_rect(tx_base + 18, ty + 6, 4, 16, 0xFF00F0FF);
        
        tx_base += 36;
        vxair_fb_fill_rect(tx_base, ty, 28, 28, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 6, ty + 10, 14, 8, 0xFF39FF14);
        vxair_fb_fill_rect(tx_base + 20, ty + 12, 2, 4, 0xFF39FF14);
        
        tx_base += 36;
        vxair_fb_fill_rect(tx_base, ty, 28, 28, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 6, ty + 11, 4, 6, 0xFF00F0FF);
        vxair_fb_fill_rect(tx_base + 10, ty + 8, 4, 12, 0xFF00F0FF);
        
        tx_base += 36;
        uint32_t clk_y = tb_y + 12;
        vxair_fb_fill_rect(tx_base, clk_y, 78, 32, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 14, clk_y + 10, 50, 2, 0xFF00F0FF);
        vxair_fb_fill_rect(tx_base + 14, clk_y + 16, 50, 2, 0xFF00F0FF);
        vxair_fb_fill_rect(tx_base + 14, clk_y + 22, 50, 2, 0xFF00F0FF);

        for (int z = 0; z < 10; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        if (g_state.launcher_open) {
            uint32_t menu_w = 460, menu_h = 260, menu_x = 12, menu_y = H - 56 - menu_h - 8;
            vxair_fb_fill_rect(menu_x - 2, menu_y - 2, menu_w + 4, menu_h + 4, 0xFF00F0FF);
            vxair_fb_fill_rect(menu_x, menu_y, menu_w, menu_h, 0xFF020617);
            
            for (int i=0; i<10; i++) {
                int col = i / 5;
                int row = i % 5;
                uint32_t item_x = menu_x + 8 + col * 220;
                uint32_t item_y = menu_y + 16 + row * 44;
                bool item_hover = (g_state.mouse_x >= (int)item_x && g_state.mouse_x <= (int)item_x + 210 && 
                                   g_state.mouse_y >= (int)item_y && g_state.mouse_y <= (int)item_y + 36);
                
                if (item_hover) vxair_fb_fill_rect(item_x, item_y, 210, 36, 0xFF1E293B);
                
                draw_app_icon(item_x + 8, item_y + 2, i, item_hover);
                
                // Text placeholder blocks
                vxair_fb_fill_rect(item_x + 48, item_y + 12, 120 + (i % 2) * 20, 4, item_hover ? 0xFF00F0FF : 0xFF94A3B8);
                vxair_fb_fill_rect(item_x + 48, item_y + 20, 60, 2, 0xFF475569);
            }
        }

        uint32_t ptr_x = g_state.mouse_x, ptr_y = g_state.mouse_y;
        vxair_fb_fill_rect(ptr_x, ptr_y, 1, 16, 0xFF405060);
        for (int i = 0; i < 11; i++) vxair_fb_fill_rect(ptr_x + i, ptr_y + i, 1, 1, 0xFF405060);
        vxair_fb_fill_rect(ptr_x + 1, ptr_y + 11, 3, 1, 0xFF405060);
        vxair_fb_fill_rect(ptr_x + 4, ptr_y + 11, 1, 5, 0xFF405060);
        vxair_fb_fill_rect(ptr_x + 7, ptr_y + 9, 1, 5, 0xFF405060);
        vxair_fb_fill_rect(ptr_x + 5, ptr_y + 15, 2, 1, 0xFF405060);
        vxair_fb_fill_rect(ptr_x + 8, ptr_y + 10, 2, 1, 0xFF405060);
        for (int i = 1; i < 11; i++) vxair_fb_fill_rect(ptr_x + 1, ptr_y + i, i - 1, 1, 0xFFFFFFFF);
        vxair_fb_fill_rect(ptr_x + 5, ptr_y + 11, 2, 4, 0xFFFFFFFF);
    }

    void vxair_compositor_main(void) {
        uint32_t W = vxair_fb_get_width();
        uint32_t H = vxair_fb_get_height();
        
        g_state.mouse_x = W / 2;
        g_state.mouse_y = H / 2;
        g_state.launcher_open = false;
        g_state.focused_window = -1;
        g_state.notes_length = 0;
        
        g_state.windows[0] = {false, VX_APP_CALCULATOR, 160, 130, 300, 390, false, 0, 0, false};
        g_state.windows[1] = {false, VX_APP_NOTES, 395, 110, 420, 420, false, 0, 0, false};
        g_state.windows[2] = {false, VX_APP_SYSMON, 235, 170, 500, 330, false, 0, 0, false};
        g_state.windows[3] = {false, VX_APP_TERMINAL, 100, 100, 600, 400, false, 0, 0, false};
        g_state.windows[4] = {false, VX_APP_FILE_MANAGER, 150, 150, 640, 480, false, 0, 0, false};
        g_state.windows[5] = {false, VX_APP_SETTINGS, 200, 120, 500, 450, false, 0, 0, false};
        g_state.windows[6] = {false, VX_APP_BROWSER, 50, 50, 800, 600, false, 0, 0, false};
        g_state.windows[7] = {false, VX_APP_MEDIA_PLAYER, 300, 200, 560, 360, false, 0, 0, false};
        g_state.windows[8] = {false, VX_APP_IMAGE_VIEWER, 250, 100, 720, 540, false, 0, 0, false};
        g_state.windows[9] = {false, VX_APP_CALENDAR, 450, 250, 380, 320, false, 0, 0, false};
        
        mouse_init();

        while (1) {
            handle_input(W, H);
            draw_polished_desktop(W, H);
            vxair_fb_flip();
            vxair_hpet_sleep_ms(16);
            g_frame++;
            if (g_frame % 60 == 0) vxair_log_info("COMPOSITOR FRAME %d", (uint32_t)g_frame);
        }
    }
}
