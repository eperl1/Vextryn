extern "C" {
#include "../../drivers/gpu/vxair_gop.h"
    extern void vxair_hpet_sleep_ms(uint32_t ms);
    extern void vxair_log_info(const char* fmt, ...);

#ifndef VXAIR_PERSISTENCE_TEST
#define VXAIR_PERSISTENCE_TEST 0
#endif

#include "app_icons.h"
#include "times_font.h"
#include "vxair_textinput.hpp"

    enum VxAppId {
        VX_APP_NONE = 0,
        VX_APP_CALCULATOR,
        VX_APP_NOTES,
        VX_APP_SYSMON,
        VX_APP_FILES,
        VX_APP_SETTINGS,
        VX_APP_TERMINAL,
        VX_APP_SNAKE,
        VX_APP_BROWSER,
        // V5 new apps
        VX_APP_MAIL,
        VX_APP_GALLERY,
        VX_APP_MEDIA_PLAYER,
        VX_APP_CLOCK,
        VX_APP_ABOUT,
        VX_APP_TASKS
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
        bool is_maximized;
        bool is_minimized;
        int saved_x, saved_y, saved_w, saved_h;
    };

    struct RamFile {
        bool in_use;
        char name[16];
        char content[512];
        int content_len;
    };

    struct VxGuiState {
        bool launcher_open;
        bool control_center_open;
        bool previous_left_down;
        int mouse_x;
        int mouse_y;
        int exact_x_fp; // fixed point x256
        int exact_y_fp; // fixed point x256
        int focused_window;
        VxWindow windows[16];

        // RAM Storage
        RamFile ram_files[10];
        
        // Settings
        int mouse_sensitivity_level; // 1 to 100
        int wallpaper_mode; // 0, 1, 2
        uint32_t accent_color;
        bool compact_taskbar;
        bool show_top_bar;
        bool show_desktop_glow;
        bool show_window_shadows;
        bool focus_dim;
        bool high_contrast;
        bool large_cursor;
        bool show_seconds;
        bool hour_24;
        bool auto_center_windows;
        bool show_close_confirm;
        
        bool wifi_enabled;
        bool bluetooth_enabled;
        bool airdrop_enabled;
        bool dnd_enabled;

        int active_menu;
        int file_selected_idx;
        bool file_preview_open;
        bool file_delete_confirm;
        bool file_rename_mode;

        bool shift_down;
        bool e0_prefix;
        bool ctrl_down;

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

        // Launcher Search
        char launcher_search[64];
        int launcher_search_len;
    };

    static VxGuiState g_state;
    static VxTextInput g_launcher_search_input;
    static uint64_t g_frame = 0;
    static int g_z_order[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

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
    static void draw_abstract_char_scaled(int x, int y, char c, uint32_t color, int scale);
    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover);
    static void save_files_to_disk();
    // V5: lerp_color now delegates to VXRender's VxColor::lerp

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
    #include "../vxrender/vxrender.hpp"
    #include "../vxui/vxui.hpp"
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
            fixed_buf[2] = 0x02;
            fixed_buf[3] = g_state.mouse_sensitivity_level;
            fixed_buf[4] = g_state.wallpaper_mode;
            fixed_buf[5] = g_state.compact_taskbar;
            write_u32_le(&fixed_buf[6], g_state.accent_color);
            fixed_buf[10] = g_state.show_top_bar;
            fixed_buf[11] = g_state.show_desktop_glow;
            fixed_buf[12] = g_state.show_window_shadows;
            fixed_buf[13] = g_state.focus_dim;
            fixed_buf[14] = g_state.high_contrast;
            fixed_buf[15] = g_state.large_cursor;
            fixed_buf[16] = g_state.show_seconds;
            fixed_buf[17] = g_state.hour_24;
            fixed_buf[18] = g_state.auto_center_windows;
            fixed_buf[19] = g_state.show_close_confirm;
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
    #include "apps/app_notes.hpp"
    #include "apps/app_calculator.hpp"
    #include "apps/app_sysmon.hpp"
    // V5 new apps
    #include "apps/app_mail.hpp"
    #include "apps/app_gallery.hpp"
    #include "apps/app_media_player.hpp"
    #include "apps/app_clock.hpp"
    #include "apps/app_about.hpp"
    #include "apps/app_tasks.hpp"
    #include "apps/app_control_center.hpp"

    // Compute the Start Menu layout as a real structure so drawing and input
    // share the exact same bounds.  This is the structural fix that prevents
    // the Browser icon (or any icon) from ever overflowing the menu.
    // V5: 14 apps in the launcher — 2-column grid layout
    const char* g_app_names[14] = {
        "Calculator", "Notes", "SysMon", "Files",
        "Settings", "Terminal", "Snake", "Browser",
        "Mail", "Gallery", "Media", "Clock",
        "About", "Tasks"
    };
    VxAppId g_app_ids[14] = {
        VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES,
        VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE, VX_APP_BROWSER,
        VX_APP_MAIL, VX_APP_GALLERY, VX_APP_MEDIA_PLAYER,
        VX_APP_CLOCK, VX_APP_ABOUT, VX_APP_TASKS
    };
    
    static bool match_search(const char* name, const char* search, int search_len) {
        if (search_len == 0) return true;
        for (int i = 0; name[i]; i++) {
            bool match = true;
            for (int j = 0; j < search_len; j++) {
                if (!name[i+j]) { match = false; break; }
                char c1 = name[i+j];
                char c2 = search[j];
                if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
                if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
                if (c1 != c2) { match = false; break; }
            }
            if (match) return true;
        }
        return false;
    }

    struct VxLauncherLayout {
        VxRect card;
        VxRect items[16];
        VxRect icon_cells[16];
        int app_indices[16];
        int item_count;
    };
    
    static VxLauncherLayout compute_launcher_layout(uint32_t W, uint32_t H, const char* search, int search_len) {
        VxLauncherLayout L;
        L.item_count = 0;
        
        for (int i = 0; i < 14; i++) {
            if (match_search(g_app_names[i], search, search_len)) {
                L.app_indices[L.item_count++] = i;
            }
        }
        
        int cols = 4;
        int rows = (L.item_count + cols - 1) / cols;
        if (rows == 0) rows = 1;
        int item_w = 80;
        int item_h = 90;
        int gap_x = 24;
        int gap_y = 24;
        int pad = 32;
        int header_h = 88;
        
        int menu_w = (item_w * cols) + (gap_x * (cols - 1)) + (pad * 2);
        int menu_h = header_h + (rows * item_h) + (gap_y * (rows - 1)) + pad;
        
        // Centered above the dock
        int menu_x = (W - menu_w) / 2;
        int menu_y = H - 56 - 20 - menu_h - 20;
        if (menu_y < 40) menu_y = 40;
        
        L.card = {menu_x, menu_y, menu_w, menu_h};
        for (int i = 0; i < L.item_count; i++) {
            int r = i / cols;
            int c = i % cols;
            int item_x = menu_x + pad + c * (item_w + gap_x);
            int item_y = menu_y + header_h + r * (item_h + gap_y);
            L.items[i] = {item_x, item_y, item_w, item_h};
            L.icon_cells[i] = {item_x + (item_w - 48)/2, item_y + 8, 48, 48};
        }
        return L;
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

    static char scancode_to_ascii(uint8_t scancode, bool shift, bool e0 = false) {
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
        if (scancode == 0x47) return e0 ? (shift ? 22 : 21) : '7'; // Home / Shift+Home / Numpad 7
        if (scancode == 0x48) return 0;   // Up arrow
        if (scancode == 0x49) return '9'; // Numpad 9
        if (scancode == 0x4B) return shift ? 19 : 17;  // Left arrow (DC1) / Shift+Left (DC3)
        if (scancode == 0x4C) return '5'; // Numpad 5
        if (scancode == 0x4D) return shift ? 20 : 18;  // Right arrow (DC2) / Shift+Right (DC4)
        if (scancode == 0x4F) return e0 ? (shift ? 24 : 23) : '1'; // End / Shift+End / Numpad 1
        if (scancode == 0x50) return 0;   // Down arrow
        if (scancode == 0x51) return '3'; // Numpad 3
        if (scancode == 0x52) return '0'; // Numpad 0
        if (scancode == 0x53) return '.'; // Numpad .
        
        return 0;
    }

    static void bring_to_front(int window_idx) {
        g_state.focused_window = window_idx;
        for (int i = 0; i < 16; i++) {
            g_state.windows[i].focused = (i == window_idx);
        }
        int pos = -1;
        for (int i = 0; i < 16; i++) {
            if (g_z_order[i] == window_idx) {
                pos = i;
                break;
            }
        }
        if (pos != -1) {
            for (int i = pos; i < 15; i++) {
                g_z_order[i] = g_z_order[i+1];
            }
            g_z_order[15] = window_idx;
        }
    }

    static void open_app(VxAppId app_id) {
        for (int i = 0; i < 16; i++) {
            if (g_state.windows[i].app == app_id) {
                bool was_closed = !g_state.windows[i].open;
                g_state.windows[i].open = true;
                if (was_closed && g_state.auto_center_windows) {
                    uint32_t W = vxair_fb_get_width();
                    uint32_t H = vxair_fb_get_height();
                    g_state.windows[i].x = (W - g_state.windows[i].w) / 2;
                    g_state.windows[i].y = (H - g_state.windows[i].h) / 2;
                    if (g_state.windows[i].y < (int)VxTheme::TOPBAR_H) g_state.windows[i].y = VxTheme::TOPBAR_H;
                }
                bring_to_front(i);
                // Reset Calculator state only when re-opening from closed
                if (was_closed && app_id == VX_APP_CALCULATOR) calc_clear();
                break;
            }
        }
    }

    static bool g_window_clicked[16] = {false};

    static void handle_input(uint32_t W, uint32_t H) {
        static uint8_t mbyte[3];
        static int cycle = 0;
        
        for (int i=0; i<16; i++) g_window_clicked[i] = false;

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
                    
                    // V2 Mouse: precision-tuned scaling with acceleration.
                    // Low speed = precise, high speed = faster. No jitter.
                    int lvl = g_state.mouse_sensitivity_level;
                    if (lvl < 1) lvl = 1; if (lvl > 100) lvl = 100;
                    // Quadratic scaling curve: 1 = ~0.125x, 50 = ~1.1x, 100 = ~4.0x
                    int scale = 128 + (lvl * lvl * 400) / 1000;
                    
                    // Acceleration: small movements get less boost, large movements get more
                    // This gives precision for small moves AND speed for large moves
                    int32_t speed_sq = rdx * rdx + rdy * rdy;
                    if (speed_sq > 256) {
                        // Fast movement — boost scale
                        scale = scale * 3 / 2;
                    } else if (speed_sq < 25) {
                        // Slow movement — reduce scale for precision
                        scale = scale * 2 / 3;
                    }
                    
                    // Sub-pixel accumulation with 10-bit precision for smoothness
                    g_state.exact_x_fp += rdx * scale;
                    g_state.exact_y_fp -= rdy * scale;
                    
                    // Use 10-bit fractional for smoother pixel rounding
                    g_state.mouse_x = (g_state.exact_x_fp + 512) >> 10;
                    g_state.mouse_y = (g_state.exact_y_fp + 512) >> 10;
                    
                    // Re-sync fixed point to actual pixel to prevent drift
                    g_state.exact_x_fp = g_state.mouse_x << 10;
                    g_state.exact_y_fp = g_state.mouse_y << 10;
                    
                    if (g_state.mouse_x < 0) { g_state.mouse_x = 0; g_state.exact_x_fp = 0; }
                    if (g_state.mouse_y < 0) { g_state.mouse_y = 0; g_state.exact_y_fp = 0; }
                    if (g_state.mouse_x > (int)W - 1) { g_state.mouse_x = W - 1; g_state.exact_x_fp = (W - 1) << 10; }
                    if (g_state.mouse_y > (int)H - 1) { g_state.mouse_y = H - 1; g_state.exact_y_fp = (H - 1) << 10; }

                    bool clicked = (left_down && !g_state.previous_left_down);
                    bool released = (!left_down && g_state.previous_left_down);
                    g_state.previous_left_down = left_down;

                    // Handle window dragging
                    if (left_down) {
                        for (int i=0; i<16; i++) {
                            if (g_state.windows[i].dragging) {
                                g_state.windows[i].x = g_state.mouse_x - g_state.windows[i].drag_offset_x;
                                g_state.windows[i].y = g_state.mouse_y - g_state.windows[i].drag_offset_y;
                                // Clamp
                                if (g_state.windows[i].y < (int)VxTheme::TOPBAR_H) g_state.windows[i].y = VxTheme::TOPBAR_H;
                                if (g_state.windows[i].y > (int)H - 80) g_state.windows[i].y = H - 80;
                                if (g_state.windows[i].x < -g_state.windows[i].w + 40) g_state.windows[i].x = -g_state.windows[i].w + 40;
                                if (g_state.windows[i].x > (int)W - 40) g_state.windows[i].x = W - 40;
                                break;
                            }
                        }
                    } else if (released) {
                        for (int i=0; i<16; i++) g_state.windows[i].dragging = false;
                    }

                    if (clicked) {
                        bool handled = false;
                        uint32_t tb_h = g_state.compact_taskbar ? 44 : VxTheme::TASKBAR_H; // V2: 56px — must match draw code
                        uint32_t tb_y = H - tb_h;
                        uint32_t mx = g_state.mouse_x;
                        uint32_t my = g_state.mouse_y;

                        // 0. Control Center button — top-right corner of top bar
                        uint32_t top_y = 12;
                        uint32_t pill_h = 32;
                        uint32_t right_pill_w = 160;
                        uint32_t right_x = W - right_pill_w - 20;
                        if (!handled && g_state.show_top_bar && mx >= right_x && mx <= right_x + right_pill_w && my >= top_y && my <= top_y + pill_h) {
                            g_state.control_center_open = !g_state.control_center_open;
                            g_state.active_menu = 0;
                            handled = true;
                        }
                        
                        // Left Pill Menus
                        if (!handled && g_state.show_top_bar && my >= top_y && my <= top_y + pill_h && mx >= 130 && mx <= 340) {
                            int clicked_menu = (mx - 130) / 52 + 1;
                            if (clicked_menu >= 1 && clicked_menu <= 4) {
                                if (g_state.active_menu == clicked_menu) g_state.active_menu = 0;
                                else g_state.active_menu = clicked_menu;
                            } else {
                                g_state.active_menu = 0;
                            }
                            handled = true;
                        } else if (!handled && g_state.active_menu > 0) {
                            // Clicked somewhere else, close the menu
                            g_state.active_menu = 0;
                        }

                        // 0.5 Control Center panel clicks
                        if (!handled && g_state.control_center_open) {
                            bool cc_consumed = draw_control_center(W, H, (int)mx, (int)my, true);
                            if (cc_consumed) {
                                handled = true;
                            } else {
                                // Click outside CC closes it
                                VxCCLayout ccl = compute_cc_layout(W, H);
                                if (!ccl.panel.contains((int)mx, (int)my)) {
                                    g_state.control_center_open = false;
                                }
                                handled = true;
                            }
                        }
                        // 1. Launcher button — must match draw code
                        int open_count_dock = 0;
                        for (int i = 0; i < 16; i++) if (g_state.windows[i].open) open_count_dock++;
                        uint32_t dock_h = 56;
                        uint32_t dock_w = 80 + (open_count_dock * 52);
                        uint32_t dock_x = (W - dock_w) / 2;
                        uint32_t dock_y = H - dock_h - 20;
                        uint32_t lx_click = dock_x + 8;
                        uint32_t ly_click = dock_y + 8;

                        if (mx >= lx_click && mx <= lx_click + 40 && my >= ly_click && my <= ly_click + 40) {
                            g_state.launcher_open = !g_state.launcher_open;
                            handled = true;
                        } 
                        // 2. Launcher open — uses the same VxLauncherLayout as draw code
                        else if (g_state.launcher_open) {
                            VxLauncherLayout L = compute_launcher_layout(W, H, g_state.launcher_search, g_state.launcher_search_len);
                            if (L.card.contains((int)mx, (int)my)) {
                                for (int i = 0; i < L.item_count; i++) {
                                    if (L.items[i].contains((int)mx, (int)my)) {
                                        open_app(g_app_ids[L.app_indices[i]]);
                                        g_state.launcher_open = false;
                                        g_state.launcher_search_len = 0;
                                        g_launcher_search_input.caret_pos = 0;
                                        g_launcher_search_input.selection_anchor = 0;
                                    }
                                }
                                handled = true;
                            } else {
                                g_state.launcher_open = false;
                            }
                        }

                        // 2.5 Taskbar apps — centered, must match draw code
                        if (!handled) {
                            uint32_t tx_base = lx_click + 64;
                            for (int i=0; i<16; i++) {
                                if (g_state.windows[i].open) {
                                    if (mx >= tx_base && mx <= tx_base + 36 && my >= dock_y + 10 && my <= dock_y + 46) {
                                        bring_to_front(i);
                                        g_state.windows[i].is_minimized = false;
                                        handled = true;
                                    }
                                    tx_base += 52;
                                }
                            }
                        }

                        // 3. Window clicks
                        if (!handled) {
                            for (int z = 15; z >= 0; z--) {
                                int i = g_z_order[z];
                                VxWindow& w = g_state.windows[i];
                                if (!w.open || w.is_minimized) continue;
                                
                                if (mx >= (uint32_t)w.x && mx <= (uint32_t)w.x + w.w && 
                                    my >= (uint32_t)w.y && my <= (uint32_t)w.y + w.h) {
                                    
                                    bring_to_front(i);
                                    handled = true;

                                    int cx = w.x + w.w - 32;
                                    int mx_btn = w.x + w.w - 62;
                                    int mn_btn = w.x + w.w - 92;
                                    int btn_y = w.y + 8;
                                    
                                    if (my >= (uint32_t)btn_y && my <= (uint32_t)btn_y + 24) {
                                        if (mx >= (uint32_t)cx && mx <= (uint32_t)cx + 24) {
                                            w.open = false;
                                            break;
                                        }
                                        if (mx >= (uint32_t)mx_btn && mx <= (uint32_t)mx_btn + 24) {
                                            if (w.is_maximized) {
                                                w.x = w.saved_x; w.y = w.saved_y; w.w = w.saved_w; w.h = w.saved_h;
                                                w.is_maximized = false;
                                            } else {
                                                w.saved_x = w.x; w.saved_y = w.y; w.saved_w = w.w; w.saved_h = w.h;
                                                w.x = 0; w.y = VxTheme::TOPBAR_H; w.w = W; w.h = H - VxTheme::TOPBAR_H - 80;
                                                w.is_maximized = true;
                                            }
                                            break;
                                        }
                                        if (mx >= (uint32_t)mn_btn && mx <= (uint32_t)mn_btn + 24) {
                                            w.is_minimized = true;
                                            break;
                                        }
                                    }
                                    
                                    // Title bar drag — V2: matches TITLE_BAR_H (38px)
                                    if (my >= (uint32_t)w.y && my <= (uint32_t)w.y + VxTheme::TITLE_BAR_H) {
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
                                for (int i=0; i<16; i++) g_state.windows[i].focused = false;
                            }
                        }
                    }
                }
            } else {
                // Keyboard
                if (data == 0xE0) {
                    g_state.e0_prefix = true;
                    continue;
                }
                bool was_e0 = g_state.e0_prefix;
                g_state.e0_prefix = false;

                if (data == 0x2A || data == 0x36) {
                    g_state.shift_down = true;
                } else if (data == 0xAA || data == 0xB6) {
                    if (!was_e0) {
                        g_state.shift_down = false;
                    }
                } else if (data == 0x1D) {
                    g_state.ctrl_down = true;
                } else if (data == 0x9D) {
                    g_state.ctrl_down = false;
                } else if ((data & 0x80) == 0) {
                    char c = scancode_to_ascii(data, g_state.shift_down, was_e0);
                    // Ctrl+A → code 25 (select_all in VxTextInput::handle_key)
                    if (g_state.ctrl_down && (c == 'a' || c == 'A')) c = 25;
                    // Ctrl+C/X/V → clipboard codes (26/28/29)
                    if (g_state.ctrl_down && c == 'c') c = 26;
                    if (g_state.ctrl_down && c == 'x') c = 28;
                    if (g_state.ctrl_down && c == 'v') c = 29;
                    // Escape closes the launcher OR clears the Calculator (when
                    // Calculator is focused). Both are handled per-app below to
                    // avoid a global side-effect; the launcher-close stays.
                    if (c == 27) {
                        g_state.launcher_open = false;
                        // If Calculator is focused, Escape also clears it.
                        if (g_state.focused_window != -1 &&
                            g_state.windows[g_state.focused_window].app == VX_APP_CALCULATOR) {
                            calc_clear();
                        }
                    } else if (g_state.launcher_open && c != 0) {
                        if (c == '\n' || c == '\r') {
                            VxLauncherLayout L = compute_launcher_layout(W, H, g_state.launcher_search, g_state.launcher_search_len);
                            if (L.item_count > 0) {
                                open_app(g_app_ids[L.app_indices[0]]);
                                g_state.launcher_open = false;
                                g_state.launcher_search_len = 0;
                                g_launcher_search_input.caret_pos = 0;
                                g_launcher_search_input.selection_anchor = 0;
                            }
                        } else {
                            g_launcher_search_input.handle_key(c);
                        }
                    } else if (c != 0) {
                        if (g_state.focused_window != -1) {
                            VxAppId app = g_state.windows[g_state.focused_window].app;
                            if (app == VX_APP_BROWSER) {
                                browser_handle_key(c);
                            } else if (app == VX_APP_NOTES) {
                                notes_handle_key(c);
                            } else if (app == VX_APP_FILES) {
                                file_handle_key(c);
                            } else if (app == VX_APP_CALCULATOR) {
                                calc_handle_key(c);
                            } else if (app == VX_APP_TERMINAL) {
                                terminal_handle_key(c);
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
        if (horizontal) vxr_fill_rect(x, y, length, 3, color);
        else vxr_fill_rect(x, y, 3, length, color);
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
                    vxr_fill_rect(x + j, y + i, 1, 1, color);
                }
            }
        }
    }

    static void draw_abstract_char_scaled(int x, int y, char c, uint32_t color, int scale) {
        if (c == ' ') return;
        uint8_t index = (uint8_t)c;
        for (int i = 0; i < 16; i++) {
            uint8_t row = times_font[index][i];
            for (int j = 0; j < 8; j++) {
                if (row & (1 << (7 - j))) {
                    vxr_fill_rect(x + (j * scale), y + (i * scale), scale, scale, color);
                }
            }
        }
    }

    // Draw a 32×32 app icon centered inside a cell of given size, clipping
    // any part that would exceed the cell.  This is the structural guarantee
    // that an icon can never overflow its container.
    static void draw_generated_icon(int cx, int cy, int cw, int ch, int app_index, bool hover) {
        if (app_index < 0 || app_index > 13) return;
        
        // Define base background gradients for all 14 apps
        uint32_t bg_top[14] = {
            0xFFFF9F0A, // 0 Calc (Orange)
            0xFFFFD60A, // 1 Notes (Yellow)
            0xFF333333, // 2 SysMon (Dark Grey)
            0xFF0A84FF, // 3 Files (Blue)
            0xFF8E8E93, // 4 Settings (Grey)
            0xFF000000, // 5 Terminal (Black)
            0xFF30D158, // 6 Snake (Green)
            0xFF5AC8FA, // 7 Browser (Light Blue)
            0xFF409CFF, // 8 Mail (Blue)
            0xFFBF5AF2, // 9 Gallery (Purple)
            0xFFFF375F, // 10 Media (Pink)
            0xFF34C759, // 11 Clock (Emerald)
            0xFFFF9F0A, // 12 About (Amber)
            0xFF32ADE6  // 13 Tasks (Cyan)
        };
        uint32_t bg_bot[14] = {
            0xFFFF8C00, // 0 Calc
            0xFFFFCC00, // 1 Notes
            0xFF1C1C1E, // 2 SysMon
            0xFF0066CC, // 3 Files
            0xFF636366, // 4 Settings
            0xFF111111, // 5 Terminal
            0xFF28A745, // 6 Snake
            0xFF007AFF, // 7 Browser
            0xFF007AFF, // 8 Mail
            0xFFAF52DE, // 9 Gallery
            0xFFFF2D55, // 10 Media
            0xFF28CD41, // 11 Clock
            0xFFFF8C00, // 12 About
            0xFF00A2FF  // 13 Tasks
        };
        
        // App icon background - soft rounded squircle
        int r = cw / 4;
        vxr_gradient_v(cx, cy, cw, ch, bg_top[app_index], bg_bot[app_index]);
        // To make it rounded, we actually want to draw a rounded rect instead of a pure block
        // We'll draw the rounded rect using vxui_draw_rounded_rect over black, but since we don't have a rounded gradient,
        // we'll just draw a flat rounded rect for now.
        vxr_fill_rect(cx, cy, cw, ch, 0x00000000); // clear
        vxui_draw_rounded_rect(cx, cy, cw, ch, r, bg_bot[app_index]); // Base flat color
        
        // Draw the inner glyph
        int center_x = cx + cw / 2;
        int center_y = cy + ch / 2;
        uint32_t fg = 0xFFFFFFFF; // White for most glyphs

        switch (app_index) {
            case 0: // Calculator
                vxr_fill_rect(center_x - 6, center_y - 2, 12, 4, fg); // -
                vxr_fill_rect(center_x - 2, center_y - 6, 4, 12, fg); // |
                break;
            case 1: // Notes
                vxr_fill_rect(cx + 8, cy + 8, cw - 16, 4, fg);
                vxr_fill_rect(cx + 8, cy + 16, cw - 20, 4, fg);
                vxr_fill_rect(cx + 8, cy + 24, cw - 24, 4, fg);
                break;
            case 2: // SysMon
                vxr_fill_rect(cx + 6, center_y, 4, 10, 0xFF30D158);
                vxr_fill_rect(cx + 14, center_y - 6, 4, 16, 0xFF30D158);
                vxr_fill_rect(cx + 22, center_y - 2, 4, 12, 0xFF30D158);
                break;
            case 3: // Files
                vxui_draw_rounded_rect(cx + 6, cy + 8, 12, 8, 2, fg);
                vxui_draw_rounded_rect(cx + 4, cy + 12, cw - 8, 14, 2, fg);
                break;
            case 4: // Settings
                vxr_circle(center_x, center_y, 8, fg);
                vxr_circle(center_x, center_y, 4, bg_bot[4]); // hole
                vxr_fill_rect(center_x - 2, cy + 4, 4, 6, fg);
                vxr_fill_rect(center_x - 2, cy + ch - 10, 4, 6, fg);
                vxr_fill_rect(cx + 4, center_y - 2, 6, 4, fg);
                vxr_fill_rect(cx + cw - 10, center_y - 2, 6, 4, fg);
                break;
            case 5: // Terminal
                vxr_fill_rect(cx + 6, cy + 8, 4, 4, 0xFF30D158);
                vxr_fill_rect(cx + 10, cy + 12, 4, 4, 0xFF30D158);
                vxr_fill_rect(cx + 6, cy + 16, 4, 4, 0xFF30D158);
                vxr_fill_rect(cx + 16, cy + 18, 10, 4, 0xFF30D158);
                break;
            case 6: // Snake
                vxui_draw_rounded_rect(cx + 8, cy + 8, cw - 16, 6, 3, fg);
                vxui_draw_rounded_rect(cx + cw - 14, cy + 8, 6, 16, 3, fg);
                vxui_draw_rounded_rect(cx + 8, cy + 18, cw - 20, 6, 3, fg);
                break;
            case 7: // Browser
                vxr_circle(center_x, center_y, 10, fg);
                vxr_circle(center_x, center_y, 8, bg_bot[7]);
                vxr_fill_rect(center_x - 1, cy + 6, 2, 20, fg);
                vxr_fill_rect(cx + 6, center_y - 1, 20, 2, fg);
                break;
            case 8: // Mail
                vxui_draw_rounded_rect(cx + 4, cy + 8, cw - 8, 16, 2, fg);
                // Simple V shape for envelope flap
                for(int i=0; i<8; i++) {
                    vxr_fill_rect(cx + 6 + i, cy + 10 + i, 2, 2, bg_bot[8]);
                    vxr_fill_rect(cx + cw - 8 - i, cy + 10 + i, 2, 2, bg_bot[8]);
                }
                break;
            case 9: // Gallery
                vxr_circle(cx + 10, cy + 10, 3, 0xFFFFD60A);
                for(int i=0; i<12; i++) {
                    vxr_fill_rect(cx + 6 + i, cy + 24 - i, 2, i+2, fg);
                    vxr_fill_rect(cx + 16 + i, cy + 24 - (8 - i), 2, (8-i)+2, fg);
                }
                break;
            case 10: // Media
                vxr_circle(center_x, center_y, 10, fg);
                // Triangle
                for(int i=0; i<6; i++) {
                    vxr_fill_rect(center_x - 2 + i, center_y - i, 2, i*2+2, bg_bot[10]);
                }
                break;
            case 11: // Clock
                vxr_circle(center_x, center_y, 12, fg);
                vxr_circle(center_x, center_y, 10, bg_bot[11]);
                vxr_fill_rect(center_x - 1, center_y - 6, 2, 7, fg);
                vxr_fill_rect(center_x - 1, center_y - 1, 6, 2, fg);
                break;
            case 12: // About
                vxr_circle(center_x, center_y, 10, fg);
                vxr_circle(center_x, center_y, 8, bg_bot[12]);
                vxr_fill_rect(center_x - 1, center_y - 5, 2, 2, fg);
                vxr_fill_rect(center_x - 1, center_y - 1, 2, 6, fg);
                break;
            case 13: // Tasks
                vxr_fill_rect(cx + 8, cy + 8, 4, 4, fg);
                vxr_fill_rect(cx + 14, cy + 8, 10, 4, fg);
                vxr_fill_rect(cx + 8, cy + 16, 4, 4, fg);
                vxr_fill_rect(cx + 14, cy + 16, 10, 4, fg);
                vxr_fill_rect(cx + 8, cy + 24, 4, 4, fg);
                vxr_fill_rect(cx + 14, cy + 24, 10, 4, fg);
                break;
        }
    }

    static void draw_app_icon_in_cell(int cx, int cy, int cw, int ch, int app_index, bool hover) {
        if (app_index < 0) return;
        draw_generated_icon(cx, cy, cw, ch, app_index, hover);
        if (hover) {
            vxui_draw_rounded_rect(cx, cy, cw, ch, cw/4, 0x44FFFFFF);
        }
    }

    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover) {
        draw_app_icon_in_cell((int)x, (int)y, 32, 32, app_index, hover);
    }



    // V5: fill_rounded_top delegates to VXRender primitive
    static inline void fill_rounded_top(int x, int y, int w, int h, uint32_t color) {
        vxr_rounded_top(x, y, w, h, 6, color);
    }

    static void draw_window(VxWindow& w, bool clicked) {
        int r = 6;
        uint32_t accent = VxTheme::accent();

        // V5: Window shadow via VXRender
        if (g_state.show_window_shadows) {
            vxr_shadow(w.x, w.y + r, w.w, w.h - r, 20);
        }

        // Window body with rounded top
        fill_rounded_top(w.x, w.y, w.w, w.h, VxTheme::SURFACE);
        // Bottom corners straight (simpler); draw bottom half as rectangle
        vxr_fill_rect(w.x, w.y + r, w.w, w.h - r, VxTheme::SURFACE);
        // Fill top corners again to ensure no holes
        fill_rounded_top(w.x, w.y, w.w, w.h, VxTheme::SURFACE);

        // V4 1px premium border
        uint32_t border_color = w.focused ? (g_state.high_contrast ? VxTheme::TEXT_PRIMARY : accent) : VxTheme::BORDER_SUBTLE;
        // Top border
        vxr_fill_rect(w.x + r, w.y, w.w - r * 2, 1, border_color);
        // Bottom border
        vxr_fill_rect(w.x, w.y + w.h - 1, w.w, 1, VxTheme::BORDER_SUBTLE);
        // Sides
        vxr_fill_rect(w.x, w.y + r, 1, w.h - r, VxTheme::BORDER_SUBTLE);
        vxr_fill_rect(w.x + w.w - 1, w.y + r, 1, w.h - r, VxTheme::BORDER_SUBTLE);
        // Top corners (rounded)
        for (int i = 0; i < r; i++) {
            vxr_fill_rect(w.x + r, w.y + i, i + 1, 1, border_color);
            vxr_fill_rect(w.x + w.w - r - i - 1, w.y + i, i + 1, 1, border_color);
        }

        // Focus glow: faint accent rim
        if (w.focused) {
            uint32_t glow = 0x18000000 | (accent & 0xFFFFFF);
            vxr_fill_rect(w.x + r, w.y - 1, w.w - r * 2, 1, glow);
            vxr_fill_rect(w.x - 1, w.y + r, 1, w.h - r, glow);
            vxr_fill_rect(w.x + w.w, w.y + r, 1, w.h - r, glow);
            vxr_fill_rect(w.x, w.y + w.h, w.w, 1, glow);
        }

        // Title bar area
        int tb_h = VxTheme::TITLE_BAR_H;
        uint32_t title_bg = w.focused ? VxTheme::SURFACE : VxTheme::BASE_DEEP;
        // Title bar rounded top
        fill_rounded_top(w.x + 1, w.y + 1, w.w - 2, tb_h, title_bg);
        // Accent indicator strip on the left
        int strip_w = 4;
        vxr_fill_rect(w.x + 1, w.y + 6, strip_w, tb_h - 12, w.focused ? accent : VxTheme::BORDER_SUBTLE);
        // Separator under title bar
        vxr_fill_rect(w.x + 1, w.y + tb_h, w.w - 2, 1, VxTheme::BORDER_SUBTLE);

        // Window title text — V5: 14 app titles
        const char* titles[] = {
            "Calculator","Notes","SysMon","Files","Settings","Terminal","Snake","Browser",
            "Mail","Gallery","Media","Clock","About","Tasks"
        };
        int title_idx = (int)w.app - 1;
        if (title_idx >= 0 && title_idx < 14) {
            const char* tn = titles[title_idx];
            int tx = w.x + 16;
            for (int i = 0; tn[i]; i++) {
                draw_abstract_char(tx + i * 10, w.y + 12, tn[i],
                                   w.focused ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_MUTED);
            }
        }

        // Window Controls (Close, Maximize, Minimize)
        int cx = w.x + w.w - 32;
        int mx_btn = w.x + w.w - 62;
        int mn_btn = w.x + w.w - 92;
        int cy = w.y + 8;
        
        bool close_hover = (g_state.mouse_x >= cx && g_state.mouse_x <= cx + 24 &&
                            g_state.mouse_y >= cy && g_state.mouse_y <= cy + 24);
        bool max_hover = (g_state.mouse_x >= mx_btn && g_state.mouse_x <= mx_btn + 24 &&
                          g_state.mouse_y >= cy && g_state.mouse_y <= cy + 24);
        bool min_hover = (g_state.mouse_x >= mn_btn && g_state.mouse_x <= mn_btn + 24 &&
                          g_state.mouse_y >= cy && g_state.mouse_y <= cy + 24);
        
        // Close Button
        uint32_t close_bg = close_hover ? VxTheme::DANGER : VxTheme::SURFACE_HIGH;
        vxr_fill_rect(cx, cy, 24, 24, close_bg);
        uint32_t x_col = close_hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
        for (int i = 0; i < 10; i++) {
            vxr_fill_rect(cx + 7 + i, cy + 7 + i, 2, 2, x_col);
            vxr_fill_rect(cx + 16 - i, cy + 7 + i, 2, 2, x_col);
        }

        // Maximize Button
        uint32_t max_bg = max_hover ? VxTheme::SURFACE : VxTheme::SURFACE_HIGH;
        vxr_fill_rect(mx_btn, cy, 24, 24, max_bg);
        uint32_t max_col = max_hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
        vxr_fill_rect(mx_btn + 7, cy + 7, 10, 10, max_col); // square outline
        vxr_fill_rect(mx_btn + 9, cy + 9, 6, 6, max_bg);    // inner hole

        // Minimize Button
        uint32_t min_bg = min_hover ? VxTheme::SURFACE : VxTheme::SURFACE_HIGH;
        vxr_fill_rect(mn_btn, cy, 24, 24, min_bg);
        uint32_t min_col = min_hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
        vxr_fill_rect(mn_btn + 7, cy + 15, 10, 2, min_col);

        VxClipRect old_clip = g_vxr_ctx.push_clip(w.x, w.y + VxTheme::TITLE_BAR_H, w.w, w.h - VxTheme::TITLE_BAR_H);

        if (w.app == VX_APP_CALCULATOR) {
            draw_app_calculator(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_NOTES) {
            draw_app_notes(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_SYSMON) {
            draw_app_sysmon(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
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
        } else if (w.app == VX_APP_MAIL) {
            draw_app_mail(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_GALLERY) {
            draw_app_gallery(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_MEDIA_PLAYER) {
            draw_app_media_player(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_CLOCK) {
            draw_app_clock(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_ABOUT) {
            draw_app_about(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_TASKS) {
            draw_app_tasks(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        }

        g_vxr_ctx.pop_clip(old_clip);
    }

    // V5: lerp_color delegates to VXRender's color utility
    static uint32_t lerp_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t) {
        return VxColor::lerp(c1, c2, t, max_t);
    }

    static void draw_polished_desktop(uint32_t W, uint32_t H) {
        // V6 Premium Shell Overhaul - Survives Grayscale
        // Deep elegant gradient background
        for (uint32_t y = 0; y < H; y++) {
            uint32_t color = VxColor::lerp(0xFF0F1115, 0xFF1A1D24, y, H);
            vxr_fill_rect(0, y, W, 1, color);
        }
        
        // Ambient soft light in center (Disabled - too slow for CPU rendering)
        if (false && g_state.show_desktop_glow) {
            uint32_t cx = W / 2, cy = H / 2;
            for (int r = 0; r < 400; r += 10) {
                uint32_t a = (40 - r / 10);
                if (a > 255) a = 0; // Prevent underflow
                uint32_t c = VxColor::with_alpha(VxTheme::accent(), a);
                vxr_circle(cx, cy, r, c);
            }
        }

        // ===== STRUCTURAL TOP BAR (Floating Pills) =====
        if (g_state.show_top_bar) {
            uint32_t top_y = 12;
            uint32_t pill_h = 32;
            
            // Left Pill: Logo & Menus
            int left_pill_w = 340;
            vxui_draw_shadow(20, top_y, left_pill_w, pill_h, 8);
            vxui_draw_rounded_rect(20, top_y, left_pill_w, pill_h, pill_h/2, VxTheme::SURFACE_HIGH);
            vxr_rounded_rect(20, top_y, left_pill_w, pill_h, pill_h/2, VxTheme::BORDER_SUBTLE);
            
            const char* logo = "Vextryn";
            for (int i = 0; logo[i]; i++) draw_abstract_char(40 + i * 10, top_y + 8, logo[i], VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(120, top_y + 8, 2, 16, VxTheme::BORDER_STRONG);
            
            const char* menus[4] = {"File", "Edit", "View", "Help"};
            int menu_x = 136;
            for (int m = 0; m < 4; m++) {
                uint32_t txt_color = (g_state.active_menu == m + 1) ? VxTheme::accent() : VxTheme::TEXT_SECONDARY;
                for (int i = 0; menus[m][i]; i++) draw_abstract_char(menu_x + i * 10, top_y + 8, menus[m][i], txt_color);
                menu_x += 52;
            }

            // Center Pill: Clock
            int center_pill_w = 120;
            int center_x = (W - center_pill_w) / 2;
            vxui_draw_shadow(center_x, top_y, center_pill_w, pill_h, 8);
            vxui_draw_rounded_rect(center_x, top_y, center_pill_w, pill_h, pill_h/2, VxTheme::SURFACE_HIGH);
            vxr_rounded_rect(center_x, top_y, center_pill_w, pill_h, pill_h/2, VxTheme::BORDER_SUBTLE);
            
            char topclock[16];
            if (g_state.show_seconds) {
                for (int i = 0; i < 9; i++) topclock[i] = "14:30:45"[i]; topclock[9] = 0;
            } else {
                for (int i = 0; i < 6; i++) topclock[i] = "14:30"[i]; topclock[6] = 0;
            }
            int tcw = 0; for (int i = 0; topclock[i]; i++) tcw += 10;
            int tcx = center_x + (center_pill_w - tcw) / 2;
            for (int i = 0; topclock[i]; i++) draw_abstract_char(tcx + i * 10, top_y + 8, topclock[i], VxTheme::TEXT_PRIMARY);

            // Right Pill: System Status & Control Center
            int right_pill_w = 160;
            int right_x = W - right_pill_w - 20;
            bool cc_hover = (g_state.mouse_x >= right_x && g_state.mouse_x <= right_x + right_pill_w && g_state.mouse_y >= (int)top_y && g_state.mouse_y <= (int)(top_y + pill_h));
            
            vxui_draw_shadow(right_x, top_y, right_pill_w, pill_h, 8);
            vxui_draw_rounded_rect(right_x, top_y, right_pill_w, pill_h, pill_h/2, cc_hover ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH);
            vxr_rounded_rect(right_x, top_y, right_pill_w, pill_h, pill_h/2, VxTheme::BORDER_SUBTLE);
            
            // WiFi
            vxr_fill_rect(right_x + 30, top_y + 16, 4, 6, VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(right_x + 36, top_y + 12, 4, 10, VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(right_x + 42, top_y + 8, 4, 14, VxTheme::TEXT_PRIMARY);
            
            // Battery
            vxr_fill_rect(right_x + 60, top_y + 10, 20, 12, VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(right_x + 82, top_y + 13, 3, 6, VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(right_x + 62, top_y + 12, 16, 8, VxTheme::SUCCESS);
            
            // Control Center Sliders
            vxr_fill_rect(right_x + 110, top_y + 8, 3, 16, VxTheme::TEXT_PRIMARY);
            vxr_fill_rect(right_x + 118, top_y + 12, 3, 12, VxTheme::TEXT_PRIMARY);
            vxr_circle(right_x + 111, top_y + 12, 3, VxTheme::accent());
            vxr_circle(right_x + 119, top_y + 18, 3, VxTheme::accent());
        }

        // ===== STRUCTURAL FLOATING DOCK =====
        int open_count = 0;
        for (int i = 0; i < 16; i++) if (g_state.windows[i].open) open_count++;
        
        uint32_t dock_h = 56;
        uint32_t dock_w = 80 + (open_count * 52); // Launcher + separator + icons
        uint32_t dock_x = (W - dock_w) / 2;
        uint32_t dock_y = H - dock_h - 20;

        // Dock Background
        vxui_draw_shadow(dock_x, dock_y, dock_w, dock_h, 12);
        vxui_draw_rounded_rect(dock_x, dock_y, dock_w, dock_h, dock_h/2, VxTheme::SURFACE_HIGH);
        vxr_rounded_rect(dock_x, dock_y, dock_w, dock_h, dock_h/2, VxTheme::BORDER_SUBTLE);
        
        // Launcher Button (Left aligned in dock)
        uint32_t lx = dock_x + 8;
        uint32_t ly = dock_y + 8;
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
        
        vxui_draw_rounded_rect(lx, ly, 40, 40, 20, launcher_hover ? VxTheme::accent_soft() : VxTheme::SURFACE);
        for (int dx = 0; dx < 2; dx++) {
            for (int dy = 0; dy < 2; dy++) {
                uint32_t dc = launcher_hover ? VxTheme::accent_glow() : VxTheme::TEXT_PRIMARY;
                vxr_fill_rect(lx + 12 + dx * 12, ly + 12 + dy * 12, 6, 6, dc);
            }
        }
        
        // Separator
        vxr_fill_rect(lx + 52, dock_y + 12, 2, dock_h - 24, VxTheme::BORDER_STRONG);

        // Taskbar Apps (Right of separator)
        uint32_t tx_base = lx + 64;
        for (int i=0; i<16; i++) {
            if (g_state.windows[i].open) {
                bool ihover = (g_state.mouse_x >= (int)tx_base && g_state.mouse_x <= (int)tx_base + 36 && g_state.mouse_y >= (int)dock_y + 10 && g_state.mouse_y <= (int)dock_y + 46);
                // Hover effect: subtle lift
                if (ihover) {
                    vxui_draw_rounded_rect(tx_base, dock_y + 8, 40, 40, 12, VxTheme::SURFACE);
                }
                
                // Focused indicator (small dot under icon)
                if (g_state.windows[i].focused) {
                    vxr_fill_rect(tx_base + 18, dock_y + 50, 4, 4, VxTheme::accent());
                }

                // Convert VxAppId (1-based) to app_index (0-based) for the icon generator
                draw_app_icon_in_cell(tx_base + 4, dock_y + 12, 32, 32, g_state.windows[i].app - 1, ihover);
                tx_base += 52;
            }
        }

        // Windows
        for (int z = 0; z < 16; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open && !g_state.windows[i].is_minimized) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        // Focus dim: darken inactive windows to emphasize focused one
        if (g_state.focus_dim) {
            for (int i = 0; i < 16; i++) {
                VxWindow& ww = g_state.windows[i];
                if (ww.open && !ww.focused) {
                    vxr_fill_rect(ww.x, ww.y, ww.w, ww.h, 0x66000000);
                }
            }
        }

        // Top Bar Dropdown Menus
        if (g_state.active_menu > 0 && g_state.active_menu <= 4) {
            int menu_w = 160;
            int menu_h = 130;
            int menu_x = 130 + (g_state.active_menu - 1) * 52;
            int menu_y = 52; // Just below top bar

            vxui_draw_shadow(menu_x, menu_y, menu_w, menu_h, 12);
            vxui_draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 8, VxTheme::SURFACE_HIGH);
            vxr_rounded_rect(menu_x, menu_y, menu_w, menu_h, 8, VxTheme::BORDER_SUBTLE);
            
            const char* dummy_items[4][4] = {
                {"New Window", "Open...", "Save", "Exit"},
                {"Undo", "Cut", "Copy", "Paste"},
                {"Zoom In", "Zoom Out", "Reset", "Full Screen"},
                {"Search", "Tutorial", "Release Notes", "About"}
            };
            
            for (int i=0; i<4; i++) {
                int item_y = menu_y + 8 + i * 30;
                bool ihover = (g_state.mouse_x >= menu_x + 8 && g_state.mouse_x <= menu_x + menu_w - 16 &&
                               g_state.mouse_y >= item_y && g_state.mouse_y <= item_y + 26);
                if (ihover) {
                    vxui_draw_rounded_rect(menu_x + 8, item_y, menu_w - 16, 26, 4, VxTheme::accent_soft());
                }
                const char* item_text = dummy_items[g_state.active_menu - 1][i];
                for (int c=0; item_text[c]; c++) {
                    draw_abstract_char(menu_x + 20 + c * 8, item_y + 8, item_text[c], ihover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);
                }
            }
        }

        // V5: Draw Control Center overlay if open
        if (g_state.control_center_open) {
            draw_control_center(W, H, g_state.mouse_x, g_state.mouse_y, false);
        }

        if (g_state.launcher_open) {
            VxLauncherLayout L = compute_launcher_layout(W, H, g_state.launcher_search, g_state.launcher_search_len);

            // ---- Massive frosted-glass drawer ----
            vxui_draw_shadow(L.card.x, L.card.y, L.card.w, L.card.h, 24);
            vxui_draw_rounded_rect(L.card.x, L.card.y, L.card.w, L.card.h, 24, VxTheme::SURFACE_HIGH);
            vxr_rounded_rect(L.card.x, L.card.y, L.card.w, L.card.h, 24, VxTheme::BORDER_SUBTLE);
            
            // Search Bar at Top
            int search_w = L.card.w - 64;
            int search_x = L.card.x + 32;
            int search_y = L.card.y + 24;
            vxui_draw_rounded_rect(search_x, search_y, search_w, 40, 20, VxTheme::BASE_DEEP);
            vxr_rounded_rect(search_x, search_y, search_w, 40, 20, VxTheme::accent());
            
            if (g_state.launcher_search_len == 0) {
                const char* hint = "Search Vextryn...";
                for (int i = 0; hint[i]; i++) {
                    draw_abstract_char(search_x + 20 + i * 8, search_y + 16, hint[i], VxTheme::TEXT_MUTED);
                }
            } else {
                for (int i = 0; i < g_state.launcher_search_len; i++) {
                    draw_abstract_char(search_x + 20 + i * 8, search_y + 16, g_state.launcher_search[i], VxTheme::TEXT_PRIMARY);
                }
            }
            if (g_frame % 60 < 30) {
                vxr_fill_rect(search_x + 20 + g_launcher_search_input.caret_pos * 8, search_y + 16, 2, 12, VxTheme::accent());
            }

            // ---- App list with real layout bounds ----
            for (int i = 0; i < L.item_count; i++) {
                int real_app_idx = L.app_indices[i];
                const VxRect& item = L.items[i];
                bool hover = item.contains(g_state.mouse_x, g_state.mouse_y);
                if (hover) {
                    vxui_draw_rounded_rect(item.x, item.y, item.w, item.h, 16, VxTheme::SURFACE);
                }
                
                // Draw icon (centered in 48x48 cell)
                draw_app_icon_in_cell(L.icon_cells[i].x, L.icon_cells[i].y, L.icon_cells[i].w, L.icon_cells[i].h, real_app_idx, hover);
                
                // Centered Label
                const char* aname = g_app_names[real_app_idx];
                int label_len = 0; for (; aname[label_len]; label_len++);
                int tx = item.x + (item.w - label_len * 8) / 2;
                int ty = item.y + item.h - 20;
                for (int j = 0; aname[j]; j++) {
                    draw_abstract_char(tx + j * 8, ty, aname[j], hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);
                }
            }
        }

        // V2 Final Cursor: crisp, bright, electric blue tip
        // Clamp so the 20×34 arrow bounding box never exceeds the framebuffer.
        int max_ptr_x = (int)W - 24;
        int max_ptr_y = (int)H - 36;
        if (max_ptr_x < 0) max_ptr_x = 0;
        if (max_ptr_y < 0) max_ptr_y = 0;
        uint32_t ptr_x = (g_state.mouse_x < 0) ? 0 : (g_state.mouse_x > max_ptr_x ? (uint32_t)max_ptr_x : (uint32_t)g_state.mouse_x);
        uint32_t ptr_y = (g_state.mouse_y < 0) ? 0 : (g_state.mouse_y > max_ptr_y ? (uint32_t)max_ptr_y : (uint32_t)g_state.mouse_y);
        if (g_state.large_cursor) {
            // Large cursor: 24x24 arrow, high visibility
            // Shadow
            for (int i = 0; i < 18; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 2, 2, 0x88000000);
            vxr_fill_rect(ptr_x + 2, ptr_y + 4, 2, 30, 0x88000000);
            // Outline
            for (int i = 0; i < 18; i++) vxr_fill_rect(ptr_x + i, ptr_y + i, 2, 2, 0xFF000000);
            vxr_fill_rect(ptr_x, ptr_y, 2, 30, 0xFF000000);
            // White fill
            for (int i = 0; i < 24; i++) vxr_fill_rect(ptr_x + 2, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            for (int i = 1; i < 14; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 2, ptr_y + 26, 6, 2, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 6, ptr_y + 26, 2, 6, 0xFFFFFFFF);
            // Accent tip
            vxr_fill_rect(ptr_x, ptr_y, 4, 4, VxTheme::accent());
        } else {
            // Normal cursor
            // Drop shadow
            vxr_fill_rect(ptr_x + 2, ptr_y + 3, 1, 18, 0x88000000);
            for (int i = 0; i < 12; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 1, 1, 0x88000000);
            // Black outline
            vxr_fill_rect(ptr_x - 1, ptr_y - 1, 1, 18, 0xFF000000);
            for (int i = 0; i < 12; i++) vxr_fill_rect(ptr_x - 1 + i, ptr_y - 1 + i, 1, 1, 0xFF000000);
            // Pure white fill
            for (int i = 0; i < 16; i++) vxr_fill_rect(ptr_x, ptr_y + i, 1, 1, 0xFFFFFFFF);
            for (int i = 1; i < 10; i++) vxr_fill_rect(ptr_x + i, ptr_y + i, 1, 1, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 1, ptr_y + 10, 5, 1, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 5, ptr_y + 11, 2, 5, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 7, ptr_y + 9, 3, 6, 0xFFFFFFFF);
            // Electric blue tip
            vxr_fill_rect(ptr_x, ptr_y, 3, 3, VxTheme::accent());
        }
    }

    void vxair_compositor_main(void) {
        vxair_log_info("COMP MARK 1: compositor entry");
        uint32_t W = vxair_fb_get_width();
        uint32_t H = vxair_fb_get_height();
        // V5: Initialize the VXRender graphics context
        g_vxr_ctx.init();
        vxair_log_info("VXRender: graphics context initialized (%dx%d)", W, H);
        
        // ---- SAFETY FALLBACK: if anything below fails, at least show visible color ----
        // V2 Fallback: deep space background with sapphire accent bars
        vxair_fb_clear(VxTheme::BASE_DEEP);
        vxr_fill_rect(W / 4, H / 4, W / 4, H / 4, VxTheme::SURFACE);
        vxr_fill_rect(W / 2, H / 2, W / 4, H / 4, VxTheme::accent());
        vxair_fb_flip();

        g_state.launcher_open = false;
        g_state.control_center_open = false;
        g_state.previous_left_down = false;
        g_state.mouse_x = W / 2;
        g_state.mouse_y = H / 2;
        g_state.exact_x_fp = g_state.mouse_x << 10;
        g_state.exact_y_fp = g_state.mouse_y << 10;
        g_state.mouse_sensitivity_level = 50;
        g_state.compact_taskbar = false;
        g_state.focused_window = -1;
        g_state.active_menu = 0;
        g_state.file_selected_idx = -1;
        g_state.file_preview_open = false;
        g_state.file_rename_mode = false;
        g_state.shift_down = false;
        g_state.e0_prefix = false;
        g_state.ctrl_down = false;
        
        g_state.accent_color = VxTheme::ACCENT;
        VxTheme::set_accent(g_state.accent_color);
        g_state.show_top_bar = true;
        g_state.show_desktop_glow = true;
        g_state.show_window_shadows = true;
        g_state.focus_dim = false;
        g_state.high_contrast = false;
        g_state.large_cursor = false;
        g_state.show_seconds = false;
        g_state.hour_24 = true;
        g_state.auto_center_windows = false;
        g_state.show_close_confirm = false;
        g_state.wallpaper_mode = 0;
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
        g_state.last_snake_move = 0;
        
        for (int i=0; i<10; i++) {
            g_state.ram_files[i].in_use = false;
            g_state.ram_files[i].content_len = 0;
            g_state.ram_files[i].name[0] = 0;
        }

        g_state.windows[0] = {false, VX_APP_CALCULATOR, 160, 130, 300, 390, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[1] = {false, VX_APP_NOTES, 395, 110, 420, 420, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[2] = {false, VX_APP_SYSMON, 235, 170, 500, 330, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[3] = {false, VX_APP_FILES, 100, 100, 600, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[4] = {false, VX_APP_SETTINGS, 150, 150, 640, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[5] = {false, VX_APP_TERMINAL, 50, 50, 600, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[6] = {false, VX_APP_SNAKE, 200, 200, 400, 428, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[7] = {false, VX_APP_BROWSER, 80, 80, 640, 480, false, 0, 0, false, false, false, 0,0,0,0};
        // V5 new app windows
        g_state.windows[8]  = {false, VX_APP_MAIL,         120, 90,  720, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[9]  = {false, VX_APP_GALLERY,      300, 100, 480, 380, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[10] = {false, VX_APP_MEDIA_PLAYER, 180, 120, 400, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[11] = {false, VX_APP_CLOCK,        250, 80,  320, 340, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[12] = {false, VX_APP_ABOUT,        200, 100, 480, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[13] = {false, VX_APP_TASKS,        300, 140, 360, 400, false, 0, 0, false, false, false, 0,0,0,0};
        for (int i = 14; i < 16; i++) g_state.windows[i] = {false, VX_APP_NONE, 0, 0, 0, 0, false, 0, 0, false, false, false, 0,0,0,0};
        
        mouse_init();

        // 1. Load Settings (Sector 0)
        uint8_t settings_buf[512] = {0};
        if (!ata_read_sector(0, settings_buf)) {
            vxair_log_info("STORAGE: no persistent ATA disk; using session defaults");
        } else {
            if (settings_buf[0] == 0xAA && settings_buf[1] == 0x55 && (settings_buf[2] == 0x01 || settings_buf[2] == 0x02)) {
                g_state.mouse_sensitivity_level = settings_buf[3];
                g_state.wallpaper_mode = settings_buf[4];
                g_state.compact_taskbar = settings_buf[5];
                g_state.accent_color = read_u32_le(&settings_buf[6]);
                VxTheme::set_accent(g_state.accent_color);
                if (settings_buf[2] == 0x02) {
                    g_state.show_top_bar = settings_buf[10];
                    g_state.show_desktop_glow = settings_buf[11];
                    g_state.show_window_shadows = settings_buf[12];
                    g_state.focus_dim = settings_buf[13];
                    g_state.high_contrast = settings_buf[14];
                    g_state.large_cursor = settings_buf[15];
                    g_state.show_seconds = settings_buf[16];
                    g_state.hour_24 = settings_buf[17];
                    g_state.auto_center_windows = settings_buf[18];
                    g_state.show_close_confirm = settings_buf[19];
                } else {
                    // Old v1 sector: apply sensible defaults for new options
                    g_state.show_top_bar = true;
                    g_state.show_desktop_glow = true;
                    g_state.show_window_shadows = true;
                    g_state.focus_dim = false;
                    g_state.high_contrast = false;
                    g_state.large_cursor = false;
                    g_state.show_seconds = false;
                    g_state.hour_24 = true;
                    g_state.auto_center_windows = false;
                    g_state.show_close_confirm = false;
                }
                if (g_state.mouse_sensitivity_level < 1) g_state.mouse_sensitivity_level = 1;
                if (g_state.mouse_sensitivity_level > 100) g_state.mouse_sensitivity_level = 100;
            }
        }

        // V5: Initialize Tasks app with sample data
        tasks_init();

        // 2. Load Files (Sector 1 for Metadata, Sectors 2-11 for content)
        uint8_t files_meta[512] = {0};
        if (ata_read_sector(1, files_meta)) {
            if (files_meta[0] == 0xAA && files_meta[1] == 0x55 && files_meta[2] == 0x01) {
                for (int i = 0; i < 10; i++) {
                    int offset = 3 + i * 21;
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

        // Initialize launcher search input
        g_launcher_search_input.init(g_state.launcher_search, &g_state.launcher_search_len, 64);
        
        vxair_log_info("COMP MARK 2: after compositor state initialization");
        vxair_log_info("GUI: compositor started at 60fps");

        g_frame = 0;
        while (1) {
            handle_input(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 3: immediately before first desktop render");
            draw_polished_desktop(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 4: immediately after first desktop render");
            vxair_fb_flip();
            if (g_frame == 0) vxair_log_info("COMP MARK 5: immediately after first framebuffer flip/present");
            vxair_hpet_sleep_ms(3);
            g_frame++;
            if (g_frame == 1) vxair_log_info("COMP MARK 6: first loop iteration reached");
            if (g_frame % 60 == 0) vxair_log_info("COMPOSITOR FRAME %u", (uint32_t)g_frame);
        }
    }
}
