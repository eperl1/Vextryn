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
        VX_APP_BROWSER,
        // V5 new apps
        VX_APP_CALENDAR,
        VX_APP_IMAGE_VIEWER,
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
        int mouse_sensitivity_level; // 1 to 5
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
        
        // File app state
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
    };

    static VxGuiState g_state;
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
    #include "apps/app_calendar.hpp"
    #include "apps/app_image_viewer.hpp"
    #include "apps/app_media_player.hpp"
    #include "apps/app_clock.hpp"
    #include "apps/app_about.hpp"
    #include "apps/app_tasks.hpp"
    #include "apps/app_control_center.hpp"

    // Compute the Start Menu layout as a real structure so drawing and input
    // share the exact same bounds.  This is the structural fix that prevents
    // the Browser icon (or any icon) from ever overflowing the menu.
    // V5: 14 apps in the launcher — 2-column grid layout
    static const int VX_APP_COUNT = 14;
    struct VxLauncherLayout {
        VxRect card;
        VxRect items[16];
        VxRect icon_cells[16];
        int item_count;
    };
    static VxLauncherLayout compute_launcher_layout(uint32_t W, uint32_t H, int count) {
        VxLauncherLayout L;
        L.item_count = count;
        // V5: 2-column grid for more apps
        int cols = 2;
        int rows = (count + cols - 1) / cols;
        int item_h = 42;
        int item_w = 140;
        int header_h = 56;
        int gap = 6;
        int menu_w = item_w * cols + gap * (cols - 1) + 24;
        int menu_h = header_h + rows * item_h + 16;
        int menu_x = 20;
        int menu_y = (int)H - VxTheme::TASKBAR_H - menu_h - 12;
        if (menu_y < (int)VxTheme::TOPBAR_H + 8) menu_y = VxTheme::TOPBAR_H + 8;
        L.card = {menu_x, menu_y, menu_w, menu_h};
        for (int i = 0; i < count; i++) {
            int r = i / cols;
            int c = i % cols;
            int item_x = menu_x + 12 + c * (item_w + gap);
            int item_y = menu_y + header_h + r * item_h;
            L.items[i] = {item_x, item_y, item_w, item_h};
            int icon_y = item_y + (item_h - 32) / 2;
            L.icon_cells[i] = {item_x + 8, icon_y, 32, 32};
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
                    if (lvl < 1) lvl = 1; if (lvl > 5) lvl = 5;
                    // Base sensitivity per level
                    int base_scale[6] = {20, 36, 52, 72, 96, 128};
                    int scale = base_scale[lvl];
                    
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
                        if (!handled && g_state.show_top_bar && mx >= W - 36 && mx <= W - 12 && my >= 4 && my <= 24) {
                            g_state.control_center_open = !g_state.control_center_open;
                            handled = true;
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
                        uint32_t tb_pad_click = 12;
                        uint32_t lx_click = tb_pad_click + 12;
                        uint32_t ly_click = tb_y + (tb_h - 6 - 36) / 2;
                        if (mx >= lx_click && mx <= lx_click + 36 && my >= ly_click && my <= ly_click + 36) {
                            g_state.launcher_open = !g_state.launcher_open;
                            handled = true;
                        } 
                        // 2. Launcher open — uses the same VxLauncherLayout as draw code
                        else if (g_state.launcher_open) {
                            VxLauncherLayout L = compute_launcher_layout(W, H, VX_APP_COUNT);
                            if (L.card.contains((int)mx, (int)my)) {
                                VxAppId app_ids[14] = {
                                    VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES,
                                    VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE, VX_APP_BROWSER,
                                    VX_APP_CALENDAR, VX_APP_IMAGE_VIEWER, VX_APP_MEDIA_PLAYER,
                                    VX_APP_CLOCK, VX_APP_ABOUT, VX_APP_TASKS
                                };
                                for (int i = 0; i < L.item_count; i++) {
                                    if (L.items[i].contains((int)mx, (int)my)) {
                                        open_app(app_ids[i]);
                                        g_state.launcher_open = false;
                                    }
                                }
                                handled = true;
                            } else {
                                g_state.launcher_open = false;
                            }
                        }

                        // 2.5 Taskbar apps — centered, must match draw code
                        if (!handled) {
                            int open_count = 0;
                            for (int i = 0; i < 16; i++) if (g_state.windows[i].open) open_count++;
                            uint32_t tb_w_click = W - 24;
                            uint32_t icon_total_w = open_count * 44 - 8;
                            uint32_t tx_base = 12 + (tb_w_click - icon_total_w) / 2;
                            uint32_t icon_y = tb_y + (tb_h - 6 - 36) / 2;
                            for (int i=0; i<16; i++) {
                                if (g_state.windows[i].open) {
                                    if (mx >= tx_base && mx <= tx_base + 36 && my >= icon_y && my <= icon_y + 36) {
                                        bring_to_front(i);
                                        handled = true;
                                    }
                                    tx_base += 44;
                                }
                            }
                        }

                        // 3. Window clicks
                        if (!handled) {
                            for (int z = 15; z >= 0; z--) {
                                int i = g_z_order[z];
                                VxWindow& w = g_state.windows[i];
                                if (!w.open) continue;
                                
                                if (mx >= (uint32_t)w.x && mx <= (uint32_t)w.x + w.w && 
                                    my >= (uint32_t)w.y && my <= (uint32_t)w.y + w.h) {
                                    
                                    bring_to_front(i);
                                    handled = true;

                                    // Close button — match the 18x18 drawn size at new position
                                    if (mx >= (uint32_t)w.x + w.w - 34 && mx <= (uint32_t)w.x + w.w - 8 && 
                                        my >= (uint32_t)w.y + 8 && my <= (uint32_t)w.y + 32) {
                                        w.open = false;
                                        break;
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

    // Draw a 32×32 app icon centered inside a cell of given size, clipping
    // any part that would exceed the cell.  This is the structural guarantee
    // that an icon can never overflow its container.
    // V5: Generate simple icons for apps 8-13 (no pixel art exists yet)
    static void draw_generated_icon(int cx, int cy, int cw, int ch, int app_index, bool hover) {
        // App-specific colors for generated icons
        uint32_t colors[6] = {
            0xFF3B8CFF, // Calendar — blue
            0xFF8B5CF6, // Image Viewer — purple
            0xFFEC4899, // Media Player — pink
            0xFF10B981, // Clock — emerald
            0xFFF59E0B, // About — amber
            0xFF06B6D4, // Tasks — cyan
        };
        int ci = app_index - 8;
        if (ci < 0 || ci > 5) ci = 0;
        uint32_t col = colors[ci];
        // Rounded background
        vxair_fb_fill_rect(cx + 2, cy + 2, cw - 4, ch - 4, col);
        vxair_fb_fill_rect(cx, cy + 4, cw, ch - 8, col);
        vxair_fb_fill_rect(cx + 4, cy, cw - 8, ch, col);
        // Simple letter indicator
        const char* letters = "CIMcat";
        char letter = letters[ci];
        int tx = cx + (cw - 8) / 2;
        int ty = cy + (ch - 16) / 2;
        draw_abstract_char(tx, ty, letter, VxTheme::TEXT_PRIMARY);
    }

    static void draw_app_icon_in_cell(int cx, int cy, int cw, int ch, int app_index, bool hover) {
        if (app_index < 0) return;
        // Apps 0-7 have pixel art icons; apps 8-13 use generated icons
        if (app_index > 7) {
            draw_generated_icon(cx, cy, cw, ch, app_index, hover);
            if (hover) {
                for (int xx = 0; xx < cw; xx++) {
                    vxair_fb_fill_rect(cx + xx, cy, 1, 1, 0x55FFFFFF);
                    vxair_fb_fill_rect(cx + xx, cy + ch - 1, 1, 1, 0x55FFFFFF);
                }
                for (int yy = 0; yy < ch; yy++) {
                    vxair_fb_fill_rect(cx, cy + yy, 1, 1, 0x55FFFFFF);
                    vxair_fb_fill_rect(cx + cw - 1, cy + yy, 1, 1, 0x55FFFFFF);
                }
            }
            return;
        }
        const uint32_t* icon_data = g_app_icons[app_index];
        // Compute top-left so the 32×32 icon is centered in the cell
        int ox = cx + (cw - 32) / 2;
        int oy = cy + (ch - 32) / 2;
        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 32; col++) {
                int px = ox + col;
                int py = oy + row;
                // Clip to the declared cell bounds — icon can NEVER escape
                if (px < cx || px >= cx + cw || py < cy || py >= cy + ch) continue;
                uint32_t color = icon_data[row * 32 + col];
                vxair_fb_fill_rect(px, py, 1, 1, color);
            }
        }
        // Hover ring, clipped to the cell
        if (hover) {
            for (int xx = 0; xx < cw && xx < 32; xx++) {
                if (xx + ox >= cx && xx + ox < cx + cw) {
                    vxair_fb_fill_rect(ox + xx, oy, 1, 1, 0x55FFFFFF);
                    vxair_fb_fill_rect(ox + xx, oy + 31, 1, 1, 0x55FFFFFF);
                }
            }
            for (int yy = 0; yy < ch && yy < 32; yy++) {
                if (yy + oy >= cy && yy + oy < cy + ch) {
                    vxair_fb_fill_rect(ox, oy + yy, 1, 1, 0x55FFFFFF);
                    vxair_fb_fill_rect(ox + 31, oy + yy, 1, 1, 0x55FFFFFF);
                }
            }
        }
    }

    static void draw_app_icon(uint32_t x, uint32_t y, int app_index, bool hover) {
        draw_app_icon_in_cell((int)x, (int)y, 32, 32, app_index, hover);
    }



    // ===== V3 rounded-rect helper: fills a rectangle with 6px rounded top corners =====
    static inline void fill_rounded_top(int x, int y, int w, int h, uint32_t color) {
        int r = 6;
        // Main body
        vxair_fb_fill_rect(x, y + r, w, h - r, color);
        vxair_fb_fill_rect(x + r, y, w - r * 2, r, color);
        // Top-left corner
        for (int i = 0; i < r; i++) {
            int width = r - i;
            vxair_fb_fill_rect(x + i, y + i, width, 1, color);
            vxair_fb_fill_rect(x, y + r, 1, 1, color);
        }
        // Top-right corner
        for (int i = 0; i < r; i++) {
            int width = r - i;
            vxair_fb_fill_rect(x + w - r + i, y + i, width, 1, color);
        }
    }

    static void draw_window(VxWindow& w, bool clicked) {
        int r = 6;
        uint32_t accent = VxTheme::accent();

        // V4 Premium window shadow with wider diffusion
        if (g_state.show_window_shadows) {
            for (int s = 0; s < 20; s++) {
                uint32_t a = (55 - s * 2);
                if (a > 55) a = 55;
                uint32_t c = 0xFF000000 | (a << 16) | (a << 8) | a;
                vxair_fb_fill_rect(w.x - s + r, w.y + w.h + s, w.w + s * 2 - r * 2, 1, c);
                vxair_fb_fill_rect(w.x + w.w + s, w.y + r, 1, w.h + s - r, c);
                vxair_fb_fill_rect(w.x - s, w.y + r, 1, w.h + s - r, c);
            }
        }

        // Window body with rounded top
        fill_rounded_top(w.x, w.y, w.w, w.h, VxTheme::SURFACE);
        // Bottom corners straight (simpler); draw bottom half as rectangle
        vxair_fb_fill_rect(w.x, w.y + r, w.w, w.h - r, VxTheme::SURFACE);
        // Fill top corners again to ensure no holes
        fill_rounded_top(w.x, w.y, w.w, w.h, VxTheme::SURFACE);

        // V4 1px premium border
        uint32_t border_color = w.focused ? (g_state.high_contrast ? VxTheme::TEXT_PRIMARY : accent) : VxTheme::BORDER_SUBTLE;
        // Top border
        vxair_fb_fill_rect(w.x + r, w.y, w.w - r * 2, 1, border_color);
        // Bottom border
        vxair_fb_fill_rect(w.x, w.y + w.h - 1, w.w, 1, VxTheme::BORDER_SUBTLE);
        // Sides
        vxair_fb_fill_rect(w.x, w.y + r, 1, w.h - r, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(w.x + w.w - 1, w.y + r, 1, w.h - r, VxTheme::BORDER_SUBTLE);
        // Top corners (rounded)
        for (int i = 0; i < r; i++) {
            vxair_fb_fill_rect(w.x + r, w.y + i, i + 1, 1, border_color);
            vxair_fb_fill_rect(w.x + w.w - r - i - 1, w.y + i, i + 1, 1, border_color);
        }

        // Focus glow: faint accent rim
        if (w.focused) {
            uint32_t glow = 0x18000000 | (accent & 0xFFFFFF);
            vxair_fb_fill_rect(w.x + r, w.y - 1, w.w - r * 2, 1, glow);
            vxair_fb_fill_rect(w.x - 1, w.y + r, 1, w.h - r, glow);
            vxair_fb_fill_rect(w.x + w.w, w.y + r, 1, w.h - r, glow);
            vxair_fb_fill_rect(w.x, w.y + w.h, w.w, 1, glow);
        }

        // Title bar area
        int tb_h = VxTheme::TITLE_BAR_H;
        uint32_t title_bg = w.focused ? VxTheme::SURFACE : VxTheme::BASE_DEEP;
        // Title bar rounded top
        fill_rounded_top(w.x + 1, w.y + 1, w.w - 2, tb_h, title_bg);
        // Accent indicator strip on the left
        int strip_w = 4;
        vxair_fb_fill_rect(w.x + 1, w.y + 6, strip_w, tb_h - 12, w.focused ? accent : VxTheme::BORDER_SUBTLE);
        // Separator under title bar
        vxair_fb_fill_rect(w.x + 1, w.y + tb_h, w.w - 2, 1, VxTheme::BORDER_SUBTLE);

        // Window title text — V5: 14 app titles
        const char* titles[] = {
            "Calculator","Notes","SysMon","Files","Settings","Terminal","Snake","Browser",
            "Calendar","Gallery","Media","Clock","About","Tasks"
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

        // V3 Close button: filled rounded square with X
        bool close_hover = (g_state.mouse_x >= w.x + w.w - 34 && g_state.mouse_x <= w.x + w.w - 8 &&
                            g_state.mouse_y >= w.y + 8 && g_state.mouse_y <= w.y + 32);
        int cx = w.x + w.w - 32, cy = w.y + 8;
        uint32_t close_bg = close_hover ? VxTheme::DANGER : VxTheme::SURFACE_HIGH;
        vxair_fb_fill_rect(cx, cy, 24, 24, close_bg);
        // Rounded corners for close button (2px radius)
        // Simple X
        uint32_t x_col = close_hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
        for (int i = 0; i < 10; i++) {
            vxair_fb_fill_rect(cx + 7 + i, cy + 7 + i, 2, 2, x_col);
            vxair_fb_fill_rect(cx + 16 - i, cy + 7 + i, 2, 2, x_col);
        }

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
        } else if (w.app == VX_APP_CALENDAR) {
            draw_app_calendar(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_IMAGE_VIEWER) {
            draw_app_image_viewer(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_MEDIA_PLAYER) {
            draw_app_media_player(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_CLOCK) {
            draw_app_clock(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_ABOUT) {
            draw_app_about(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_TASKS) {
            draw_app_tasks(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
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
        // V4 Desktop: premium aurora-like gradient with subtle noise
        if (g_state.wallpaper_mode == 2) {
            vxair_fb_fill_rect(0, 0, W, H, VxTheme::BASE_DEEP);
        } else if (g_state.wallpaper_mode == 1) {
            vxair_fb_fill_rect(0, 0, W, H, VxTheme::BASE_DEEP);
            for (uint32_t y = 24; y < H; y += 48) {
                for (uint32_t x = 24; x < W; x += 48) {
                    vxair_fb_fill_rect(x, y, 2, 2, VxTheme::BORDER_SUBTLE);
                }
            }
        } else {
            // Aurora gradient: smooth blend across the whole screen
            for (uint32_t y = 0; y < H; y++) {
                uint32_t base = lerp_color(VxTheme::BASE_DEEP, VxTheme::BASE_MID, y, H);
                uint32_t aurora = lerp_color(VxTheme::BASE_DEEP, 0xFF102040, y, H);
                uint32_t color = lerp_color(aurora, base, y, H);
                vxair_fb_fill_rect(0, y, W, 1, color);
            }
            // Large soft aurora glow blobs
            if (g_state.show_desktop_glow) {
                uint32_t cx1 = W / 3, cx2 = W * 2 / 3, cy = H / 4;
                for (int r = 0; r < 180; r += 4) {
                    uint32_t a = (60 - r / 6);
                    if (a > 40) a = 40;
                    uint32_t c = (a << 24) | 0x3B8CFF;
                    vxair_fb_fill_rect(cx1 - r, cy - r, r * 2, 1, c);
                    vxair_fb_fill_rect(cx2 - r, cy + 40 - r, r * 2, 1, c);
                }
            }
        }
        // Extra ambient glow in the lower third (independent of aurora blobs)
        if (g_state.show_desktop_glow && g_state.wallpaper_mode != 1) {
            uint32_t cgx = W / 2, cgy = H * 3 / 4;
            for (int r = 0; r < 140; r += 8) {
                uint32_t a = (50 - r / 4) / 4;
                if (a > 12) a = 12;
                uint32_t c = (a << 24) | 0x3B8CFF;
                vxair_fb_fill_rect(cgx - r, cgy - r, r * 2, 1, c);
            }
        }

        // ===== macOS-STYLE TOP MENU BAR =====
        if (g_state.show_top_bar) {
        uint32_t top_h = VxTheme::TOPBAR_H; // 28px
        // Top bar background — brighter than desktop, with bottom border
        vxair_fb_fill_rect(0, 0, W, top_h, VxTheme::BASE_DEEP);
        vxair_fb_fill_rect(0, top_h - 1, W, 1, VxTheme::BORDER_STRONG);
        // VAir OS logo on left
        const char* logo = "V Air";
        for (int i = 0; logo[i]; i++) {
            draw_abstract_char(12 + i * 10, 8, logo[i], VxTheme::accent_glow());
        }
        // Menu items
        const char* menus[5] = {"File", "Edit", "View", "Window", "Help"};
        int menu_x = 70;
        for (int m = 0; m < 5; m++) {
            bool mhover = (g_state.mouse_x >= menu_x && g_state.mouse_x <= menu_x + 50 && g_state.mouse_y < (int)top_h);
            for (int i = 0; menus[m][i]; i++) {
                draw_abstract_char(menu_x + i * 10, 8, menus[m][i],
                                   mhover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);
            }
            menu_x += 56;
        }
        // Right side: clock + status icons
        char topclock[16];
        if (g_state.hour_24) {
            if (g_state.show_seconds) {
                for (int i = 0; i < 9; i++) topclock[i] = "14:30:45"[i];
                topclock[9] = 0;
            } else {
                for (int i = 0; i < 6; i++) topclock[i] = "14:30"[i];
                topclock[6] = 0;
            }
        } else {
            if (g_state.show_seconds) {
                for (int i = 0; i < 11; i++) topclock[i] = "02:30:45 PM"[i];
                topclock[11] = 0;
            } else {
                for (int i = 0; i < 8; i++) topclock[i] = "02:30 PM"[i];
                topclock[8] = 0;
            }
        }
        int tcw = 0;
        for (int i = 0; topclock[i]; i++) tcw += 10;
        int tcx = W - tcw - 16;
        for (int i = 0; topclock[i]; i++) {
            draw_abstract_char(tcx + i * 10, 8, topclock[i], VxTheme::TEXT_PRIMARY);
        }
        // WiFi icon
        vxair_fb_fill_rect(tcx - 24, 10, 4, 8, VxTheme::accent_glow());
        vxair_fb_fill_rect(tcx - 18, 8, 4, 10, VxTheme::accent_glow());
        vxair_fb_fill_rect(tcx - 12, 6, 4, 12, VxTheme::accent_glow());
        // Battery icon
        vxair_fb_fill_rect(tcx - 44, 10, 14, 8, VxTheme::SUCCESS);
        vxair_fb_fill_rect(tcx - 30, 12, 2, 4, VxTheme::SUCCESS);
        // V5: Control Center button (top-right corner)
        bool cc_hover = (g_state.mouse_x >= (int)W - 36 && g_state.mouse_x <= (int)W - 12 && g_state.mouse_y >= 4 && g_state.mouse_y <= 24);
        vxair_fb_fill_rect(W - 36, 4, 24, 20, cc_hover ? VxTheme::SURFACE_HIGH : VxTheme::BASE_DEEP);
        // Sliders icon (two small bars)
        vxair_fb_fill_rect(W - 30, 8, 3, 12, cc_hover ? VxTheme::accent() : VxTheme::TEXT_SECONDARY);
        vxair_fb_fill_rect(W - 24, 10, 3, 10, cc_hover ? VxTheme::accent() : VxTheme::TEXT_SECONDARY);
        vxair_fb_fill_rect(W - 18, 6, 3, 14, cc_hover ? VxTheme::accent() : VxTheme::TEXT_SECONDARY);
        }

        uint32_t tb_h = g_state.compact_taskbar ? 44 : VxTheme::TASKBAR_H;
        uint32_t tb_y = H - tb_h;
        // V4 Floating glass taskbar
        uint32_t tb_pad = 16;
        uint32_t tb_w = W - tb_pad * 2;
        // Soft shadow
        for (int s = 0; s < 12; s++) {
            uint32_t a = (50 - s * 4) / 2;
            if (a > 30) a = 30;
            uint32_t c = 0xFF000000 | (a << 16) | (a << 8) | a;
            vxair_fb_fill_rect(tb_pad - s, tb_y + s, tb_w + s * 2, 1, c);
        }
        // Taskbar body — glass panel
        vxair_fb_fill_rect(tb_pad, tb_y, tb_w, tb_h - 6, VxTheme::GLASS_TINT);
        vxair_fb_fill_rect(tb_pad, tb_y + tb_h - 6, tb_w, 1, VxTheme::BORDER_SUBTLE);
        // Premium accent line at top
        vxair_fb_fill_rect(tb_pad + 12, tb_y + 3, tb_w - 24, 2, VxTheme::accent());

        uint32_t lx = tb_pad + 12, ly = tb_y + (tb_h - 6 - 36) / 2;
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 36 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 36);
        
        // V3 Launcher button: clean, centered in floating taskbar
        if (launcher_hover) {
            // Soft glow
            for (int s = 0; s < 6; s++) {
                uint32_t a = 30 - s * 5;
                vxair_fb_fill_rect(lx - s, ly - s, 36 + s*2, 36 + s*2, 0x10000000 | (a << 16) | (a << 8) | a);
            }
        }
        // Button with sapphire accent when hovered
        vxair_fb_fill_rect(lx, ly, 36, 36, launcher_hover ? VxTheme::accent_soft() : VxTheme::SURFACE_HIGH);
        vxair_fb_fill_rect(lx + 1, ly + 1, 34, 34, g_state.launcher_open ? VxTheme::accent_soft() : VxTheme::BASE_DEEP);
        // Four dots in grid — more modern
        for (int dx = 0; dx < 2; dx++) {
            for (int dy = 0; dy < 2; dy++) {
                uint32_t dc = launcher_hover ? VxTheme::accent_glow() : VxTheme::TEXT_SECONDARY;
                vxair_fb_fill_rect(lx + 10 + dx * 10, ly + 10 + dy * 10, 6, 6, dc);
            }
        }

        // V5 Taskbar apps — centered in the floating taskbar
        int open_count = 0;
        for (int i = 0; i < 16; i++) if (g_state.windows[i].open) open_count++;
        uint32_t icon_total_w = open_count * 44 - 8;
        uint32_t tx_base = tb_pad + (tb_w - icon_total_w) / 2;
        
        for (int i=0; i<16; i++) {
            if (g_state.windows[i].open) {
                uint32_t icon_y = tb_y + (tb_h - 6 - 36) / 2;
                bool hover = (g_state.mouse_x >= (int)tx_base && g_state.mouse_x <= (int)tx_base + 36 && 
                              g_state.mouse_y >= (int)icon_y && g_state.mouse_y <= (int)icon_y + 36);
                if (hover) {
                    vxair_fb_fill_rect(tx_base - 3, icon_y - 3, 42, 42, VxTheme::OVERLAY);
                    vxair_fb_fill_rect(tx_base, icon_y, 36, 36, VxTheme::SURFACE_HIGH);
                }
                
                if (g_state.windows[i].focused) {
                    vxair_fb_fill_rect(tx_base + 8, tb_y + tb_h - 10, 20, 3, VxTheme::accent());
                }
                
                int app_idx = g_state.windows[i].app - 1;
                draw_app_icon(tx_base + 2, icon_y + 2, app_idx, hover);

                tx_base += 44;
            }
        }
        
        tx_base = W - 200;
        uint32_t ty = tb_y + (tb_h - 28) / 2;
        // V2 System tray — refined icons with glass backgrounds
        vxair_fb_fill_rect(tx_base, ty, 28, 28, VxTheme::SURFACE_HIGH);
        vxair_fb_fill_rect(tx_base + 1, ty + 1, 26, 26, VxTheme::SURFACE);            vxair_fb_fill_rect(tx_base + 8, ty + 14, 4, 8, VxTheme::accent_glow());
            vxair_fb_fill_rect(tx_base + 14, ty + 10, 4, 12, VxTheme::accent_glow());
        
        tx_base += 32;
        vxair_fb_fill_rect(tx_base, ty, 28, 28, VxTheme::SURFACE_HIGH);
        vxair_fb_fill_rect(tx_base + 1, ty + 1, 26, 26, VxTheme::SURFACE);
        vxair_fb_fill_rect(tx_base + 6, ty + 10, 16, 8, VxTheme::SUCCESS);
        vxair_fb_fill_rect(tx_base + 20, ty + 12, 2, 4, VxTheme::SUCCESS);
        
        tx_base += 32;
        // V2 Clock — glass panel
        vxair_fb_fill_rect(tx_base, ty, 150, 28, VxTheme::SURFACE_HIGH);
        vxair_fb_fill_rect(tx_base + 1, ty + 1, 148, 26, VxTheme::SURFACE);
        
        char dt[20];
        if (g_state.show_seconds) {
            if (g_state.hour_24) {
                for (int i = 0; i < 9; i++) dt[i] = "JUL28 14:30:45"[i];
                dt[9] = 0;
            } else {
                for (int i = 0; i < 12; i++) dt[i] = "JUL28 02:30:45P"[i];
                dt[12] = 0;
            }
        } else {
            if (g_state.hour_24) {
                for (int i = 0; i < 6; i++) dt[i] = "JUL28 14:30"[i];
                dt[6] = 0;
            } else {
                for (int i = 0; i < 9; i++) dt[i] = "JUL28 02:30P"[i];
                dt[9] = 0;
            }
        }
        for (int i=0; dt[i] != 0; i++) {
            draw_abstract_char(tx_base + 8 + i * 10, ty + 8, dt[i], VxTheme::TEXT_PRIMARY);
        }

        for (int z = 0; z < 16; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        // Focus dim: darken inactive windows to emphasize focused one
        if (g_state.focus_dim) {
            for (int i = 0; i < 16; i++) {
                VxWindow& ww = g_state.windows[i];
                if (ww.open && !ww.focused) {
                    vxair_fb_fill_rect(ww.x, ww.y, ww.w, ww.h, 0x66000000);
                }
            }
        }

        // V5: Draw Control Center overlay if open
        if (g_state.control_center_open) {
            draw_control_center(W, H, g_state.mouse_x, g_state.mouse_y, false);
        }

        if (g_state.launcher_open) {
            const char* app_names[14] = {
                "Calculator", "Notes", "SysMon", "Files",
                "Settings", "Terminal", "Snake", "Browser",
                "Calendar", "Gallery", "Media", "Clock",
                "About", "Tasks"
            };
            VxLauncherLayout L = compute_launcher_layout(W, H, VX_APP_COUNT);

            // ---- Premium frosted-glass card ----
            vxui_draw_shadow(L.card.x, L.card.y, L.card.w, L.card.h, 14);
            VxPanel card_panel{L.card.x, L.card.y, L.card.w, L.card.h, 0, VxTheme::GLASS_TINT};
            card_panel.draw();

            // ---- Header / search area ----
            vxair_fb_fill_rect(L.card.x, L.card.y, L.card.w, 64, VxTheme::SURFACE);
            vxair_fb_fill_rect(L.card.x, L.card.y + 64, L.card.w, 1, VxTheme::BORDER_SUBTLE);
            // Title
            const char* launcher_title = "Start";
            for (int i = 0; launcher_title[i]; i++) {
                draw_abstract_char(L.card.x + 20 + i * 10, L.card.y + 14, launcher_title[i], VxTheme::TEXT_PRIMARY);
            }
            // Search pill
            int search_x = L.card.x + 20;
            int search_y = L.card.y + 34;
            int search_w = L.card.w - 40;
            vxair_fb_fill_rect(search_x, search_y, search_w, 24, VxTheme::BASE_DEEP);
            vxair_fb_fill_rect(search_x, search_y, search_w, 1, VxTheme::BORDER_BRIGHT);
            vxair_fb_fill_rect(search_x, search_y + 23, search_w, 1, VxTheme::BORDER_SUBTLE);
            vxair_fb_fill_rect(search_x, search_y, 1, 24, VxTheme::BORDER_SUBTLE);
            vxair_fb_fill_rect(search_x + search_w - 1, search_y, 1, 24, VxTheme::BORDER_SUBTLE);
            const char* hint = "Search apps...";
            for (int i = 0; hint[i]; i++) {
                draw_abstract_char(search_x + 12 + i * 8, search_y + 7, hint[i], VxTheme::TEXT_MUTED);
            }

            // ---- App list with real layout bounds ----
            for (int i = 0; i < L.item_count; i++) {
                const VxRect& item = L.items[i];
                bool hover = item.contains(g_state.mouse_x, g_state.mouse_y);
                if (hover) {
                    vxair_fb_fill_rect(item.x, item.y, item.w, item.h, VxTheme::OVERLAY);
                    vxair_fb_fill_rect(item.x, item.y + 10, 4, item.h - 20, VxTheme::accent());
                }
                // Icon cell: 32×32, clipped to the menu, cannot overflow
                const VxRect& ic = L.icon_cells[i];
                vxair_fb_fill_rect(ic.x - 2, ic.y - 2, ic.w + 4, ic.h + 4, VxTheme::SURFACE_HIGH);
                draw_app_icon(ic.x, ic.y, i, hover);
                // Label
                for (int j = 0; app_names[i][j]; j++) {
                    draw_abstract_char(item.x + 54 + j * 10, item.y + 19, app_names[i][j],
                                       hover ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY);
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
            for (int i = 0; i < 18; i++) vxair_fb_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 2, 2, 0x88000000);
            vxair_fb_fill_rect(ptr_x + 2, ptr_y + 4, 2, 30, 0x88000000);
            // Outline
            for (int i = 0; i < 18; i++) vxair_fb_fill_rect(ptr_x + i, ptr_y + i, 2, 2, 0xFF000000);
            vxair_fb_fill_rect(ptr_x, ptr_y, 2, 30, 0xFF000000);
            // White fill
            for (int i = 0; i < 24; i++) vxair_fb_fill_rect(ptr_x + 2, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            for (int i = 1; i < 14; i++) vxair_fb_fill_rect(ptr_x + 2 + i, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            vxair_fb_fill_rect(ptr_x + 2, ptr_y + 26, 6, 2, 0xFFFFFFFF);
            vxair_fb_fill_rect(ptr_x + 6, ptr_y + 26, 2, 6, 0xFFFFFFFF);
            // Accent tip
            vxair_fb_fill_rect(ptr_x, ptr_y, 4, 4, VxTheme::accent());
        } else {
            // Normal cursor
            // Drop shadow
            vxair_fb_fill_rect(ptr_x + 2, ptr_y + 3, 1, 18, 0x88000000);
            for (int i = 0; i < 12; i++) vxair_fb_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 1, 1, 0x88000000);
            // Black outline
            vxair_fb_fill_rect(ptr_x - 1, ptr_y - 1, 1, 18, 0xFF000000);
            for (int i = 0; i < 12; i++) vxair_fb_fill_rect(ptr_x - 1 + i, ptr_y - 1 + i, 1, 1, 0xFF000000);
            // Pure white fill
            for (int i = 0; i < 16; i++) vxair_fb_fill_rect(ptr_x, ptr_y + i, 1, 1, 0xFFFFFFFF);
            for (int i = 1; i < 10; i++) vxair_fb_fill_rect(ptr_x + i, ptr_y + i, 1, 1, 0xFFFFFFFF);
            vxair_fb_fill_rect(ptr_x + 1, ptr_y + 10, 5, 1, 0xFFFFFFFF);
            vxair_fb_fill_rect(ptr_x + 5, ptr_y + 11, 2, 5, 0xFFFFFFFF);
            vxair_fb_fill_rect(ptr_x + 7, ptr_y + 9, 3, 6, 0xFFFFFFFF);
            // Electric blue tip
            vxair_fb_fill_rect(ptr_x, ptr_y, 3, 3, VxTheme::accent());
        }
    }

    void vxair_compositor_main(void) {
        vxair_log_info("COMP MARK 1: compositor entry");
        uint32_t W = vxair_fb_get_width();
        uint32_t H = vxair_fb_get_height();
        
        // ---- SAFETY FALLBACK: if anything below fails, at least show visible color ----
        // V2 Fallback: deep space background with sapphire accent bars
        vxair_fb_clear(VxTheme::BASE_DEEP);
        vxair_fb_fill_rect(W / 4, H / 4, W / 4, H / 4, VxTheme::SURFACE);
        vxair_fb_fill_rect(W / 2, H / 2, W / 4, H / 4, VxTheme::accent());
        vxair_fb_flip();

        g_state.launcher_open = false;
        g_state.control_center_open = false;
        g_state.previous_left_down = false;
        g_state.mouse_x = W / 2;
        g_state.mouse_y = H / 2;
        g_state.exact_x_fp = g_state.mouse_x << 10;
        g_state.exact_y_fp = g_state.mouse_y << 10;
        g_state.mouse_sensitivity_level = 3;
        g_state.compact_taskbar = false;
        g_state.focused_window = -1;
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

        g_state.windows[0] = {false, VX_APP_CALCULATOR, 160, 130, 300, 390, false, 0, 0, false};
        g_state.windows[1] = {false, VX_APP_NOTES, 395, 110, 420, 420, false, 0, 0, false};
        g_state.windows[2] = {false, VX_APP_SYSMON, 235, 170, 500, 330, false, 0, 0, false};
        g_state.windows[3] = {false, VX_APP_FILES, 100, 100, 600, 400, false, 0, 0, false};
        g_state.windows[4] = {false, VX_APP_SETTINGS, 150, 150, 640, 480, false, 0, 0, false};
        g_state.windows[5] = {false, VX_APP_TERMINAL, 50, 50, 600, 400, false, 0, 0, false};
        g_state.windows[6] = {false, VX_APP_SNAKE, 200, 200, 400, 428, false, 0, 0, false};
        g_state.windows[7] = {false, VX_APP_BROWSER, 80, 80, 640, 480, false, 0, 0, false};
        // V5 new app windows
        g_state.windows[8]  = {false, VX_APP_CALENDAR,     120, 90,  400, 340, false, 0, 0, false};
        g_state.windows[9]  = {false, VX_APP_IMAGE_VIEWER, 300, 100, 480, 380, false, 0, 0, false};
        g_state.windows[10] = {false, VX_APP_MEDIA_PLAYER, 180, 120, 400, 400, false, 0, 0, false};
        g_state.windows[11] = {false, VX_APP_CLOCK,        250, 80,  320, 340, false, 0, 0, false};
        g_state.windows[12] = {false, VX_APP_ABOUT,        200, 100, 480, 400, false, 0, 0, false};
        g_state.windows[13] = {false, VX_APP_TASKS,        300, 140, 360, 400, false, 0, 0, false};
        for (int i = 14; i < 16; i++) g_state.windows[i] = {false, VX_APP_NONE, 0, 0, 0, 0, false, 0, 0, false};
        
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
                if (g_state.mouse_sensitivity_level > 5) g_state.mouse_sensitivity_level = 5;
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
            vxair_hpet_sleep_ms(16);
            g_frame++;
            if (g_frame == 1) vxair_log_info("COMP MARK 6: first loop iteration reached");
            if (g_frame % 60 == 0) vxair_log_info("COMPOSITOR FRAME %u", (uint32_t)g_frame);
        }
    }
}
