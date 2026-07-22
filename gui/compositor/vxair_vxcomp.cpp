extern "C" {
#include "../../drivers/gpu/vxair_gop.h"
    extern void vxair_hpet_sleep_ms(uint32_t ms);
    extern void vxair_log_info(const char* fmt, ...);

#ifndef VXAIR_PERSISTENCE_TEST
#define VXAIR_PERSISTENCE_TEST 0
#endif

#include "app_icons.h"
#include "times_font.h"

    enum VxAppId {
        VX_APP_NONE = 0,
        VX_APP_CALCULATOR,
        VX_APP_NOTES,
        VX_APP_SYSMON,
        VX_APP_FILES,
        VX_APP_SETTINGS,
        VX_APP_TERMINAL,
        VX_APP_SNAKE,
        VX_APP_BROWSER
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

    struct RamFile {
        bool in_use;
        char name[16];
        char content[512];
        int content_len;
    };

    struct VxGuiState {
        bool launcher_open;
        bool previous_left_down;
        int mouse_x;
        int mouse_y;
        int exact_x_fp; // fixed point x256
        int exact_y_fp; // fixed point x256
        int focused_window;
        VxWindow windows[8];

        // RAM Storage
        RamFile ram_files[10];
        
        // Settings
        int mouse_sensitivity_level; // 1 to 5
        int wallpaper_mode; // 0, 1, 2
        uint32_t accent_color;
        bool compact_taskbar;
        
        // File app state
        int file_selected_idx;
        bool file_preview_open;
        bool file_delete_confirm;
        bool file_rename_mode;

        char notes[1024];
        int notes_length;
        bool shift_down;

        int calc_accumulator;
        int calc_pending_value;
        char calc_operator;
        bool calc_replace_display;
        bool calc_error;

        // Terminal
        char term_buffer[64];
        int term_len;
        char term_output[512];
        int term_out_len;

        // Snake
        int snake_x[100];
        int snake_y[100];
        int snake_len;
        int snake_dir;
        int food_x;
        int food_y;
        bool snake_dead;
        uint64_t last_snake_move;
    };

    static VxGuiState g_state;
    static uint64_t g_frame = 0;
    static int g_z_order[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    static inline int clamp(int v, int min_v, int max_v) {
        if (v < min_v) return min_v;
        if (v > max_v) return max_v;
        return v;
    }

    // Forward declarations
    static uint32_t lerp_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t);
    static void draw_polished_desktop(uint32_t W, uint32_t H);
    static void draw_window(VxWindow& w, bool clicked);
    static void draw_digit(int x, int y, int digit, uint32_t color);
    static void draw_number(int x, int y, int num, uint32_t color);
    static void draw_segment(int x, int y, int length, bool horizontal, uint32_t color);
    static void draw_abstract_char(int x, int y, char c, uint32_t color);
    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover);
    static void save_files_to_disk();

    static inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
        return ret;
    }
    static inline void outb(uint16_t port, uint8_t val) {
        __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    }

    // Include native app components
    #include "font8x8.h"
    #include "ata_storage.hpp"
    #include "apps/app_file_manager.hpp"

    static inline void write_u32_le(uint8_t* p, uint32_t value) {
        p[0] = value & 0xFF;
        p[1] = (value >> 8) & 0xFF;
        p[2] = (value >> 16) & 0xFF;
        p[3] = (value >> 24) & 0xFF;
    }

    static inline uint32_t read_u32_le(const uint8_t* p) {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }

    static bool intercepted_settings_write(uint32_t lba, uint8_t* buf) {
        if (lba == 0) {
            uint8_t fixed_buf[512] = {0};
            fixed_buf[0] = 0xAA;
            fixed_buf[1] = 0x55;
            fixed_buf[2] = 0x01;
            fixed_buf[3] = g_state.mouse_sensitivity_level;
            fixed_buf[4] = 0;
            fixed_buf[5] = g_state.compact_taskbar;
            write_u32_le(&fixed_buf[6], g_state.accent_color);
            return (ata_write_sector)(0, fixed_buf);
        }
        return (ata_write_sector)(lba, buf);
    }

#define ata_write_sector intercepted_settings_write
    #include "apps/app_settings.hpp"
#undef ata_write_sector

    #include "apps/app_terminal.hpp"
    #include "apps/app_snake.hpp"
    #include "apps/app_browser.hpp"


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

    static bool write_warned = false;
    static void save_files_to_disk() {
        uint8_t files_meta[512] = {0};
        files_meta[0] = 0xAA;
        files_meta[1] = 0x55;
        files_meta[2] = 0x01; // Version 1
        bool write_success = true;
        for (int i = 0; i < 10; i++) {
            int offset = 3 + i * 21;
            files_meta[offset] = g_state.ram_files[i].in_use;
            for (int j = 0; j < 16; j++) {
                files_meta[offset + 1 + j] = g_state.ram_files[i].name[j];
            }
            int safe_len = g_state.ram_files[i].content_len;
            if (safe_len > 511) safe_len = 511;
            *(int*)(&files_meta[offset + 17]) = safe_len;
            
            if (g_state.ram_files[i].in_use) {
                uint8_t content_buf[512] = {0};
                for (int j = 0; j < safe_len; j++) {
                    content_buf[j] = g_state.ram_files[i].content[j];
                }
                if (!ata_write_sector(2 + i, content_buf)) {
                    write_success = false;
                }
            }
        }
        if (!ata_write_sector(1, files_meta)) {
            write_success = false;
        }
        if (!write_success && !write_warned) {
            vxair_log_info("STORAGE: write unavailable; changes are session-only");
            write_warned = true;
        }
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
        if (scancode == 0x0C) return shift ? '_' : '-';
        if (scancode == 0x0D) return shift ? '+' : '=';
        if (scancode == 0x1A) return shift ? '{' : '[';
        if (scancode == 0x1B) return shift ? '}' : ']';
        if (scancode == 0x27) return shift ? ':' : ';';
        if (scancode == 0x28) return shift ? '"' : '\'';
        if (scancode == 0x29) return shift ? '~' : '`';
        if (scancode == 0x2B) return shift ? '|' : '\\';
        if (scancode == 0x33) return shift ? '<' : ',';
        if (scancode == 0x34) return shift ? '>' : '.';
        if (scancode == 0x35) return shift ? '?' : '/';
        if (scancode == 0x4A) return '-'; // Numpad -
        if (scancode == 0x4E) return '+'; // Numpad +
        if (scancode == 0x37) return '*'; // Numpad * (or print screen)
        if (scancode == 0x47) return '7'; // Numpad 7
        if (scancode == 0x48) return 0;   // Up arrow
        if (scancode == 0x49) return '9'; // Numpad 9
        if (scancode == 0x4B) return 17;  // Left arrow (DC1)
        if (scancode == 0x4C) return '5'; // Numpad 5
        if (scancode == 0x4D) return 18;  // Right arrow (DC2)
        if (scancode == 0x4F) return '1'; // Numpad 1
        if (scancode == 0x50) return 0;   // Down arrow
        if (scancode == 0x51) return '3'; // Numpad 3
        if (scancode == 0x52) return '0'; // Numpad 0
        if (scancode == 0x53) return '.'; // Numpad .
        
        return 0;
    }

    static void bring_to_front(int window_idx) {
        g_state.focused_window = window_idx;
        for (int i = 0; i < 8; i++) {
            g_state.windows[i].focused = (i == window_idx);
        }
        int pos = -1;
        for (int i = 0; i < 8; i++) {
            if (g_z_order[i] == window_idx) {
                pos = i;
                break;
            }
        }
        if (pos != -1) {
            for (int i = pos; i < 7; i++) {
                g_z_order[i] = g_z_order[i+1];
            }
            g_z_order[7] = window_idx;
        }
    }

    static void open_app(VxAppId app_id) {
        for (int i = 0; i < 8; i++) {
            if (g_state.windows[i].app == app_id) {
                g_state.windows[i].open = true;
                bring_to_front(i);
                break;
            }
        }
    }

    static bool g_window_clicked[8] = {false};

    static void handle_input(uint32_t W, uint32_t H) {
        static uint8_t mbyte[3];
        static int cycle = 0;
        
        for (int i=0; i<8; i++) g_window_clicked[i] = false;

        while ((inb(0x64) & 1) != 0) {
            uint8_t status = inb(0x64);
            uint8_t data = inb(0x60);
            
            if (status & 0x20) {
                // Mouse
                if (cycle == 0 && (data & 0x08) == 0) continue; // Bit 3 sync check
                
                mbyte[cycle++] = data;
                if (cycle == 3) {
                    cycle = 0;
                    uint8_t state = mbyte[0];
                    if (state & 0xC0) continue; // Ignore overflow packets
                    
                    bool left_down = (state & 0x01) != 0;
                    
                    int32_t rdx = mbyte[1]; if (state & 0x10) rdx -= 256;
                    int32_t rdy = mbyte[2]; if (state & 0x20) rdy -= 256;
                    
                    int scale_lut[6] = {32, 16, 32, 48, 64, 96};
                    int lvl = g_state.mouse_sensitivity_level;
                    if (lvl < 1) lvl = 1; if (lvl > 5) lvl = 5;
                    int scale = scale_lut[lvl];
                    
                    if (g_state.exact_x_fp == 0 && g_state.exact_y_fp == 0) {
                        g_state.exact_x_fp = g_state.mouse_x << 8;
                        g_state.exact_y_fp = g_state.mouse_y << 8;
                    }
                    
                    g_state.exact_x_fp += rdx * scale;
                    g_state.exact_y_fp -= rdy * scale;
                    
                    g_state.mouse_x = g_state.exact_x_fp >> 8;
                    g_state.mouse_y = g_state.exact_y_fp >> 8;
                    
                    if (g_state.mouse_x < 0) { g_state.mouse_x = 0; g_state.exact_x_fp = 0; }
                    if (g_state.mouse_y < 0) { g_state.mouse_y = 0; g_state.exact_y_fp = 0; }
                    if (g_state.mouse_x > (int)W - 1) { g_state.mouse_x = W - 1; g_state.exact_x_fp = (W - 1) << 8; }
                    if (g_state.mouse_y > (int)H - 1) { g_state.mouse_y = H - 1; g_state.exact_y_fp = (H - 1) << 8; }

                    bool clicked = (left_down && !g_state.previous_left_down);
                    bool released = (!left_down && g_state.previous_left_down);
                    g_state.previous_left_down = left_down;

                    // Handle window dragging
                    if (left_down) {
                        for (int i=0; i<8; i++) {
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
                        for (int i=0; i<8; i++) g_state.windows[i].dragging = false;
                    }

                    if (clicked) {
                        bool handled = false;
                        uint32_t tb_h = g_state.compact_taskbar ? 40 : 56;
                        uint32_t tb_y = H - tb_h;
                        uint32_t mx = g_state.mouse_x;
                        uint32_t my = g_state.mouse_y;

                        // 1. Launcher button
                        if (mx >= 12 && mx <= 52 && my >= tb_y + 8 && my <= tb_y + 48) {
                            g_state.launcher_open = !g_state.launcher_open;
                            handled = true;
                        } 
                        // 2. Launcher open
                        else if (g_state.launcher_open) {
                            uint32_t menu_w = 200;
                            uint32_t menu_h = 8 * 40 + 20; // 7 apps
                            uint32_t menu_x = 12, menu_y = H - tb_h - menu_h - 8;
                            if (mx >= menu_x && mx <= menu_x + menu_w && my >= menu_y && my <= menu_y + menu_h) {
                                VxAppId app_ids[8] = {VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES, VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE, VX_APP_BROWSER};
                                for (int i=0; i<8; i++) {
                                    uint32_t item_y = menu_y + 10 + i * 40;
                                    if (mx >= 20 && mx <= 200 && my >= item_y && my <= item_y + 30) {
                                        open_app(app_ids[i]);
                                        g_state.launcher_open = false;
                                    }
                                }
                                handled = true;
                            } else {
                                g_state.launcher_open = false;
                            }
                        }

                        // 2.5 Taskbar apps
                        if (!handled) {
                            uint32_t tx_base = 60;
                            for (int i=0; i<8; i++) {
                                if (g_state.windows[i].open) {
                                    if (mx >= tx_base && mx <= tx_base + 32 && my >= tb_y + 4 && my <= tb_y + 36) {
                                        bring_to_front(i);
                                        handled = true;
                                    }
                                    tx_base += 40;
                                }
                            }
                        }

                        // 3. Window clicks
                        if (!handled) {
                            for (int z = 7; z >= 0; z--) {
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
                                for (int i=0; i<8; i++) g_state.windows[i].focused = false;
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
                        if (g_state.focused_window != -1) {
                            VxAppId app = g_state.windows[g_state.focused_window].app;
                            if (app == VX_APP_BROWSER) {
                                browser_handle_key(c);
                            } else if (app == VX_APP_NOTES) {
                                if (c == '\b') {
                                    if (g_state.notes_length > 0) g_state.notes_length--;
                                } else if (g_state.notes_length < 1023) {
                                    g_state.notes[g_state.notes_length++] = c;
                                }
                            } else if (app == VX_APP_FILES) {
                                if (g_state.file_selected_idx >= 0 && g_state.file_selected_idx < 10) {
                                    RamFile& rf = g_state.ram_files[g_state.file_selected_idx];
                                    if (rf.in_use) {
                                        if (g_state.file_rename_mode) {
                                            int len = 0; while (rf.name[len] != 0 && len < 15) len++;
                                            if (c == '\b') { if (len > 0) rf.name[len-1] = 0; }
                                            else if (c != '\n' && len < 15) { rf.name[len] = c; rf.name[len+1] = 0; }
                                            save_files_to_disk();
                                        } else if (g_state.file_preview_open) {
                                            if (c == '\b') {
                                                if (rf.content_len > 0) rf.content_len--;
                                            } else if (rf.content_len < 511) {
                                                rf.content[rf.content_len++] = c;
                                            }
                                            save_files_to_disk();
                                        }
                                    }
                                }
                            } else if (app == VX_APP_CALCULATOR) {
                                if (c >= '0' && c <= '9') {
                                    if (g_state.calc_replace_display || g_state.calc_error) {
                                        g_state.calc_accumulator = c - '0';
                                        g_state.calc_replace_display = false;
                                        g_state.calc_error = false;
                                    } else {
                                        if (g_state.calc_accumulator < 10000000) {
                                            g_state.calc_accumulator = g_state.calc_accumulator * 10 + (c - '0');
                                        }
                                    }
                                } else if (c == '+' || c == '-' || c == '*' || c == '/') {
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
                                    g_state.calc_accumulator = g_state.calc_pending_value; // Display intermediate result
                                    g_state.calc_operator = c;
                                    g_state.calc_replace_display = true;
                                } else if (c == '=' || c == '\n') {
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
                                } else if (c == '\b') {
                                    g_state.calc_accumulator /= 10;
                                }
                            } else if (app == VX_APP_TERMINAL) {
                                if (c == '\b') {
                                    if (g_state.term_len > 0) {
                                        g_state.term_len--;
                                        g_state.term_buffer[g_state.term_len] = 0;
                                    }
                                } else if (c == '\n') {
                                    // Handle command
                                    g_state.term_buffer[g_state.term_len] = 0;
                                    if (g_state.term_len > 0) {
                                        if (g_state.term_buffer[0] == 'h') {
                                            const char* msg = "cmds: help, clear, whoami, version, date";
                                            for(int i=0; msg[i]&&i<511; i++) g_state.term_output[i] = msg[i];
                                            g_state.term_out_len = 40;
                                        } else if (g_state.term_buffer[0] == 'c') {
                                            g_state.term_out_len = 0;
                                        } else if (g_state.term_buffer[0] == 'w') {
                                            const char* msg = "vxair-root";
                                            for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                                            g_state.term_out_len = 10;
                                        } else if (g_state.term_buffer[0] == 'v') {
                                            const char* msg = "VextrynAir OS v0.1";
                                            for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                                            g_state.term_out_len = 18;
                                        } else if (g_state.term_buffer[0] == 'd') {
                                            const char* msg = "Mon Jul 20 2026";
                                            for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                                            g_state.term_out_len = 15;
                                        } else {
                                            const char* msg = "cmd not found";
                                            for(int i=0; msg[i]; i++) g_state.term_output[i] = msg[i];
                                            g_state.term_out_len = 13;
                                        }
                                    }
                                    g_state.term_len = 0;
                                    g_state.term_buffer[0] = 0;
                                } else if (g_state.term_len < 63) {
                                    g_state.term_buffer[g_state.term_len++] = c;
                                    g_state.term_buffer[g_state.term_len] = 0;
                                }
                            } else if (app == VX_APP_SNAKE) {
                                if (c == 'w' || c == 'W') g_state.snake_dir = 0;
                                else if (c == 's' || c == 'S') g_state.snake_dir = 1;
                                else if (c == 'a' || c == 'A') g_state.snake_dir = 2;
                                else if (c == 'd' || c == 'D') g_state.snake_dir = 3;
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
        uint8_t index = (uint8_t)c;
        for (int i = 0; i < 16; i++) {
            uint8_t row = times_font[index][i];
            for (int j = 0; j < 8; j++) {
                if (row & (1 << (7 - j))) {
                    vxair_fb_fill_rect(x + j, y + i, 1, 1, color);
                }
            }
        }
    }

    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover) {
        if (app_index < 0 || app_index > 6) return;
        const uint32_t* icon_data = g_app_icons[app_index];
        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 32; col++) {
                uint32_t color = icon_data[row * 32 + col];
                // Apply a simple brightness modifier on hover if we want, or just draw
                // To keep it simple, just draw
                vxair_fb_fill_rect(x + col, y + row, 1, 1, color);
            }
        }
        
        // If hovered, maybe draw a subtle highlight outline
        if (hover) {
            vxair_fb_fill_rect(x, y, 32, 1, 0x44FFFFFF);
            vxair_fb_fill_rect(x, y+31, 32, 1, 0x44FFFFFF);
            vxair_fb_fill_rect(x, y, 1, 32, 0x44FFFFFF);
            vxair_fb_fill_rect(x+31, y, 1, 32, 0x44FFFFFF);
        }
    }

    static void draw_window(VxWindow& w, bool clicked) {
        // 1px border
        vxair_fb_fill_rect(w.x - 1, w.y - 1, w.w + 2, w.h + 2, w.focused ? 0xFF00F0FF : 0xFF334155);
        vxair_fb_fill_rect(w.x, w.y, w.w, w.h, 0xFF1E293B); // Fill
        
        vxair_fb_fill_rect(w.x, w.y, w.w, 24, w.focused ? 0xFF00F0FF : 0xFF334155);
        vxair_fb_fill_rect(w.x, w.y + 1, w.w, 23, 0xFF0F172A);
        
        bool close_hover = (g_state.mouse_x >= w.x + w.w - 20 && g_state.mouse_x <= w.x + w.w - 4 && 
                            g_state.mouse_y >= w.y + 4 && g_state.mouse_y <= w.y + 20);
        vxair_fb_fill_rect(w.x + w.w - 20, w.y + 4, 16, 16, close_hover ? 0xFFEF4444 : 0xFF991B1B);

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
            vxair_fb_fill_rect(w.x + 30, w.y + 40, w.w - 60, 20, 0xFF0F172A);
            vxair_fb_fill_rect(w.x + 30, w.y + 40, (w.w - 60) * 45 / 100, 20, 0xFF3B82F6);
            draw_abstract_char(w.x + 30, w.y + 70, 'R', 0xFFF8FAFC);
            draw_abstract_char(w.x + 42, w.y + 70, 'A', 0xFFF8FAFC);
            draw_abstract_char(w.x + 54, w.y + 70, 'M', 0xFFF8FAFC);
            
            vxair_fb_fill_rect(w.x + 30, w.y + 110, w.w - 60, 20, 0xFF0F172A);
            vxair_fb_fill_rect(w.x + 30, w.y + 110, (w.w - 60) * 15 / 100, 20, 0xFF10B981);
            draw_abstract_char(w.x + 30, w.y + 140, 'C', 0xFFF8FAFC);
            draw_abstract_char(w.x + 42, w.y + 140, 'P', 0xFFF8FAFC);
            draw_abstract_char(w.x + 54, w.y + 140, 'U', 0xFFF8FAFC);
        } else if (w.app == VX_APP_TERMINAL) {
            draw_app_terminal(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_SNAKE) {
            draw_app_snake(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_FILES) {
            draw_app_file_manager(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_SETTINGS) {
            draw_app_settings(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_BROWSER) {
            draw_app_browser(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
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

        uint32_t tb_h = g_state.compact_taskbar ? 40 : 56;
        uint32_t tb_y = H - tb_h;
        vxair_fb_fill_rect(0, tb_y - 1, W, 1, g_state.accent_color);
        vxair_fb_fill_rect(0, tb_y, W, tb_h, 0xFF020617);

        uint32_t lx = 12, ly = tb_y + (tb_h - 32) / 2;
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 32 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 32);
        
        vxair_fb_fill_rect(lx, ly, 32, 32, launcher_hover ? g_state.accent_color : 0xFF1E293B);
        vxair_fb_fill_rect(lx + 1, ly + 1, 30, 30, g_state.launcher_open ? 0xFF0F172A : 0xFF020617);

        uint32_t tx_base = 60;
        
        // Draw Taskbar apps
        for (int i=0; i<8; i++) {
            if (g_state.windows[i].open) {
                bool hover = (g_state.mouse_x >= (int)tx_base && g_state.mouse_x <= (int)tx_base + 32 && 
                              g_state.mouse_y >= (int)tb_y + 4 && g_state.mouse_y <= (int)tb_y + 36);
                if (hover) vxair_fb_fill_rect(tx_base, tb_y + (tb_h - 32)/2, 32, 32, 0xFF334155);
                
                if (g_state.windows[i].focused) {
                    vxair_fb_fill_rect(tx_base, tb_y + tb_h - 4, 32, 4, g_state.accent_color);
                }
                
                int app_idx = g_state.windows[i].app - 1;
                draw_app_icon(tx_base, tb_y + (tb_h - 32)/2, app_idx, hover);

                tx_base += 40;
            }
        }
        
        tx_base = W - 180;
        uint32_t ty = tb_y + (tb_h - 24) / 2;
        vxair_fb_fill_rect(tx_base, ty, 24, 24, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 6, ty + 12, 4, 6, g_state.accent_color);
        vxair_fb_fill_rect(tx_base + 12, ty + 8, 4, 10, g_state.accent_color);
        
        tx_base += 28;
        vxair_fb_fill_rect(tx_base, ty, 24, 24, 0xFF1E293B);
        vxair_fb_fill_rect(tx_base + 4, ty + 8, 14, 8, 0xFF10B981);
        vxair_fb_fill_rect(tx_base + 18, ty + 10, 2, 4, 0xFF10B981);
        
        tx_base += 28;
        vxair_fb_fill_rect(tx_base, ty, 140, 24, 0xFF0F172A);
        
        // Very basic static date/time for now since we don't have full RTC/Timer mapped to g_state
        const char* dt = "JUL 20 10:42";
        for (int i=0; dt[i] != 0; i++) {
            draw_abstract_char(tx_base + 4 + i * 10, ty + 6, dt[i], 0xFFFFFFFF);
        }

        for (int z = 0; z < 8; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        if (g_state.launcher_open) {
            uint32_t menu_w = 200;
            uint32_t menu_h = 8 * 40 + 20; // 7 apps
            uint32_t menu_y = tb_y - menu_h - 8;
            vxair_fb_fill_rect(12, menu_y, menu_w, menu_h, 0xEE0F172A); // slight transparency
            vxair_fb_fill_rect(12, menu_y, menu_w, 1, g_state.accent_color);
            vxair_fb_fill_rect(12, menu_y, 1, menu_h, g_state.accent_color);
            vxair_fb_fill_rect(12 + menu_w, menu_y, 1, menu_h, g_state.accent_color);
            
            const char* app_names[8] = {"Calculator", "Notes", "SysMon", "Files", "Settings", "Terminal", "Snake", "Browser"};
            
            for (int i=0; i<8; i++) {
                uint32_t item_y = menu_y + 10 + i * 40;
                bool hover = (g_state.mouse_x >= 20 && g_state.mouse_x <= 200 && g_state.mouse_y >= (int)item_y && g_state.mouse_y <= (int)item_y + 36);
                if (hover) {
                    vxair_fb_fill_rect(20, item_y, 184, 36, 0x66334155);
                }
                draw_app_icon(24, item_y + 2, i, hover);
                for (int j=0; app_names[i][j]; j++) {
                    draw_abstract_char(64 + j*10, item_y + 12, app_names[i][j], 0xFFF8FAFC);
                }
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
        vxair_log_info("COMP MARK 1: compositor entry");
        uint32_t W = vxair_fb_get_width();
        uint32_t H = vxair_fb_get_height();
        
        g_state.launcher_open = false;
        g_state.previous_left_down = false;
        g_state.mouse_x = W / 2;
        g_state.mouse_y = H / 2;
        g_state.exact_x_fp = 0;
        g_state.exact_y_fp = 0;
        g_state.mouse_sensitivity_level = 3;
        g_state.compact_taskbar = true;
        g_state.focused_window = -1;
        g_state.file_selected_idx = -1;
        g_state.file_preview_open = false;
        g_state.file_rename_mode = false;
        g_state.notes_length = 0;
        
        g_state.accent_color = 0xFF06B6D4;
        g_state.term_buffer[0] = 0;
        g_state.term_len = 0;
        g_state.term_out_len = 0;
        g_state.snake_len = 3;
        g_state.snake_x[0] = 10; g_state.snake_y[0] = 10;
        g_state.snake_x[1] = 9; g_state.snake_y[1] = 10;
        g_state.snake_x[2] = 8; g_state.snake_y[2] = 10;
        g_state.snake_dir = 3;
        g_state.food_x = 15; g_state.food_y = 15;
        g_state.snake_dead = false;
        
        for (int i=0; i<10; i++) {
            g_state.ram_files[i].in_use = false;
            g_state.ram_files[i].content_len = 0;
            g_state.ram_files[i].name[0] = 0;
        }

        g_state.windows[0] = {false, VX_APP_CALCULATOR, 160, 130, 300, 390, false, 0, 0, false};
        g_state.windows[1] = {false, VX_APP_NOTES, 395, 110, 420, 420, false, 0, 0, false};
        g_state.windows[2] = {false, VX_APP_SYSMON, 235, 170, 500, 330, false, 0, 0, false};
        g_state.windows[3] = {false, VX_APP_FILES, 100, 100, 600, 400, false, 0, 0, false};
        g_state.windows[4] = {false, VX_APP_SETTINGS, 150, 150, 640, 480, false, 0, 0, false};
        g_state.windows[5] = {false, VX_APP_TERMINAL, 50, 50, 600, 400, false, 0, 0, false};
        g_state.windows[6] = {false, VX_APP_SNAKE, 200, 200, 400, 428, false, 0, 0, false};
        g_state.windows[7] = {false, VX_APP_BROWSER, 80, 80, 640, 480, false, 0, 0, false};
        
        mouse_init();

        // 1. Load Settings (Sector 0)
        uint8_t settings_buf[512] = {0};
        if (!ata_read_sector(0, settings_buf)) {
            vxair_log_info("STORAGE: no persistent ATA disk; using session defaults");
        } else {
            if (settings_buf[0] == 0xAA && settings_buf[1] == 0x55 && settings_buf[2] == 0x01) { // Signature
                g_state.mouse_sensitivity_level = settings_buf[3];
                g_state.compact_taskbar = settings_buf[5];
                g_state.accent_color = read_u32_le(&settings_buf[6]);
                
                // Clamp sensitivity
                if (g_state.mouse_sensitivity_level < 1) g_state.mouse_sensitivity_level = 1;
                if (g_state.mouse_sensitivity_level > 5) g_state.mouse_sensitivity_level = 5;

                if (g_state.mouse_sensitivity_level == 3 && g_state.compact_taskbar == 1 && g_state.accent_color == 0x1A2B3C4D) {
                    vxair_log_info("PERSIST TEST: cold read PASS");
                } else {
                    vxair_log_info("PERSIST TEST: cold read FAIL");
                }
            }
        }

        // 2. Load Files (Sector 1 for Metadata, Sectors 2-11 for content)
        uint8_t files_meta[512] = {0};
        if (ata_read_sector(1, files_meta)) {
            if (files_meta[0] == 0xAA && files_meta[1] == 0x55 && files_meta[2] == 0x01) { // Signature and Version
                for (int i = 0; i < 10; i++) {
                    int offset = 3 + i * 21; // 1 (in_use) + 16 (name) + 4 (len)
                    g_state.ram_files[i].in_use = files_meta[offset];
                    for (int j = 0; j < 16; j++) {
                        g_state.ram_files[i].name[j] = files_meta[offset + 1 + j];
                    }
                    
                    int len = *(int*)(&files_meta[offset + 17]);
                    if (len < 0) len = 0;
                    if (len > 511) len = 511;
                    g_state.ram_files[i].content_len = len;
                    
                    if (g_state.ram_files[i].in_use) {
                        uint8_t content_buf[512] = {0};
                        if (ata_read_sector(2 + i, content_buf)) {
                            for (int j = 0; j < len; j++) {
                                g_state.ram_files[i].content[j] = content_buf[j];
                            }
                        } else {
                            g_state.ram_files[i].in_use = false;
                        }
                    }
                }
            }
        }

        vxair_log_info("COMP MARK 2: after compositor state initialization");
        vxair_log_info("COMP STORAGE SAFE: initialization continued");

#if VXAIR_PERSISTENCE_TEST == 1
        g_state.mouse_sensitivity_level = 3;
        g_state.compact_taskbar = 1;
        g_state.accent_color = 0x1A2B3C4D;
        vxair_log_info("PERSIST TEST: write requested");
        intercepted_settings_write(0, nullptr);
        
        uint8_t readback[512] = {0};
        if (ata_read_sector(0, readback)) {
            if (readback[0] == 0xAA && readback[1] == 0x55 && readback[2] == 0x01 &&
                readback[3] == 0x04 && readback[4] == 0x00 && readback[5] == 0x01 &&
                readback[6] == 0x4D && readback[7] == 0x3C && readback[8] == 0x2B && readback[9] == 0x1A) {
                
                bool zeroes_ok = true;
                for (int i=10; i<512; i++) { if (readback[i] != 0) { zeroes_ok = false; break; } }
                if (zeroes_ok) {
                    vxair_log_info("PERSIST TEST: immediate readback PASS");
                } else {
                    vxair_log_info("PERSIST TEST: immediate readback FAIL");
                }
            } else {
                vxair_log_info("PERSIST TEST: immediate readback FAIL");
            }
        } else {
            vxair_log_info("PERSIST TEST: immediate readback FAIL");
        }
#endif

        while (1) {
            handle_input(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 3: immediately before first desktop render");
            draw_polished_desktop(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 4: immediately after first desktop render");
            vxair_fb_flip();
            if (g_frame == 0) vxair_log_info("COMP MARK 5: immediately after first framebuffer flip/present");
            vxair_hpet_sleep_ms(16);
            g_frame++;
            if (g_frame == 1) vxair_log_info("COMP MARK 6: first loop iteration reached");
            if (g_frame % 60 == 0) vxair_log_info("COMPOSITOR FRAME %d", (uint32_t)g_frame);
        }
    }
}
