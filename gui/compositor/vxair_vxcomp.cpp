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
        VX_APP_MAIL,
        VX_APP_GALLERY,
        VX_APP_MEDIA_PLAYER,
        VX_APP_CLOCK,
        VX_APP_ABOUT,
        VX_APP_TASKS,
        VX_APP_CODE_EDITOR,
        VX_APP_DOC_VIEWER,
        VX_APP_ARCHIVE,
        VX_APP_STORE,
        VX_APP_SHOT
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
        VxWindow windows[20];

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
    static int g_z_order[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

    // ===== Static wallpaper cache: the desktop wallpaper (gradient + glows + grid) is
    // invariant across frames, so render it once and blit it each frame instead of
    // recomputing per-pixel lerps and alpha-blended glow circles on every frame. =====
    extern void* vxair_kmalloc(size_t size);
    static uint32_t* g_wp_cache = nullptr;
    static bool g_wp_cached = false;
    static int g_wp_w = 0;

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
    #include "../vxui/vx_text.hpp"
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
    #include "apps/app_mail.hpp"
    #include "apps/app_gallery.hpp"
    #include "apps/app_media_player.hpp"
    #include "apps/app_clock.hpp"
    #include "apps/app_about.hpp"
    #include "apps/app_tasks.hpp"
    #include "apps/app_control_center.hpp"
    #include "apps/app_code_editor.hpp"
    #include "apps/app_doc_viewer.hpp"
    #include "apps/app_archive_manager.hpp"
    #include "apps/app_software_center.hpp"
    #include "apps/app_screenshot.hpp"

    const char* g_app_names[19] = {
        "Calculator", "Notes", "SysMon", "Files",
        "Settings", "Terminal", "Snake", "Browser",
        "Mail", "Gallery", "Media", "Clock",
        "About", "Tasks", "Editor", "Docs",
        "Archive", "Software", "Shot"
    };
    VxAppId g_app_ids[19] = {
        VX_APP_CALCULATOR, VX_APP_NOTES, VX_APP_SYSMON, VX_APP_FILES,
        VX_APP_SETTINGS, VX_APP_TERMINAL, VX_APP_SNAKE, VX_APP_BROWSER,
        VX_APP_MAIL, VX_APP_GALLERY, VX_APP_MEDIA_PLAYER,
        VX_APP_CLOCK, VX_APP_ABOUT, VX_APP_TASKS,
        VX_APP_CODE_EDITOR, VX_APP_DOC_VIEWER, VX_APP_ARCHIVE,
        VX_APP_STORE, VX_APP_SHOT
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
        VxRect items[24];
        VxRect icon_cells[24];
        int app_indices[24];
        int item_count;
    };
    
    static VxLauncherLayout compute_launcher_layout(uint32_t W, uint32_t H, const char* search, int search_len) {
        VxLauncherLayout L;
        L.item_count = 0;
        
        for (int i = 0; i < 19; i++) {
            if (match_search(g_app_names[i], search, search_len)) {
                L.app_indices[L.item_count++] = i;
            }
        }
        
        int cols = 6;
        int rows = (L.item_count + cols - 1) / cols;
        if (rows == 0) rows = 1;
        int item_w = 96;
        int item_h = 96;
        int gap_x = 2;
        int gap_y = 2;
        int pad = 14;
        int header_h = 68;
        int footer_h = 58;
        int footer_gap = 10;

        int grid_w = (item_w * cols) + (gap_x * (cols - 1));
        int menu_w = 660;
        if (menu_w < grid_w + pad * 2) menu_w = grid_w + pad * 2;
        int menu_h = header_h + (rows * item_h) + (gap_y * (rows - 1)) + footer_gap + footer_h + pad;

        // Centered above the dock
        int menu_x = (W - menu_w) / 2;
        int menu_y = H - 56 - 20 - menu_h - 20;
        if (menu_y < 40) menu_y = 40;

        L.card = {menu_x, menu_y, menu_w, menu_h};
        int grid_x = menu_x + (menu_w - grid_w) / 2;
        for (int i = 0; i < L.item_count; i++) {
            int r = i / cols;
            int c = i % cols;
            int item_x = grid_x + c * (item_w + gap_x);
            int item_y = menu_y + header_h + r * (item_h + gap_y);
            L.items[i] = {item_x, item_y, item_w, item_h};
            L.icon_cells[i] = {item_x + (item_w - 44)/2, item_y + 6, 44, 44};
        }
        return L;
    }

    static int find_taskbar_window_idx(VxAppId app_id) {
        for (int i = 0; i < 16; i++) {
            if (g_state.windows[i].open && g_state.windows[i].app == app_id) return i;
        }
        return -1;
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
                    // Keep pointer movement 1:1 with PS/2 deltas so the visible
                    // pointer stays aligned with hit targets in QEMU.
                    (void)g_state.mouse_sensitivity_level;
                    g_state.exact_x_fp += (rdx << 10);
                    g_state.exact_y_fp -= (rdy << 10);

                    g_state.mouse_x = (g_state.exact_x_fp + 512) >> 10;
                    g_state.mouse_y = (g_state.exact_y_fp + 512) >> 10;
                    
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
                        vxair_log_info("VX: click (%u,%u) cc_open=%d launcher=%d", mx, my, (int)g_state.control_center_open, (int)g_state.launcher_open);

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

                        // 0.1 Tray WiFi button → Quick settings (Open Design: #btn-tray opens .popover).
                        // Hit region spans the whole right tray icon cluster (WiFi/Bluetooth/Volume/Battery).
                        {
                            uint32_t dock_h = VxTheme::TASKBAR_H;
                            uint32_t tray_y = H - dock_h + 8;
                            uint32_t rx = W - 12 - 152;
                            if (!handled && mx >= rx - 8 && mx <= rx + 92 && my >= tray_y + 2 && my <= tray_y + 42) {
                                g_state.control_center_open = !g_state.control_center_open;
                                g_state.active_menu = 0;
                                handled = true;
                                vxair_log_info("VX: tray click toggled CC -> %d", (int)g_state.control_center_open);
                            }
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
                        uint32_t dock_h = VxTheme::TASKBAR_H;
                        uint32_t dock_y = H - dock_h;
                        uint32_t lx_click = VxTheme::TASKBAR_PAD;
                        uint32_t ly_click = dock_y + 8;
                        uint32_t app_x = lx_click + 52;

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

                        // 2.5 Taskbar apps — fixed buttons, must match draw code
                        if (!handled) {
                            const VxAppId task_apps[6] = {
                                VX_APP_FILES, VX_APP_TERMINAL, VX_APP_BROWSER,
                                VX_APP_CODE_EDITOR, VX_APP_MAIL, VX_APP_MEDIA_PLAYER
                            };
                            uint32_t tx_base = app_x;
                            for (int i = 0; i < 6; i++) {
                                if (mx >= tx_base && mx <= tx_base + 40 && my >= ly_click && my <= ly_click + 40) {
                                    open_app(task_apps[i]);
                                    handled = true;
                                    break;
                                }
                                tx_base += 48;
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

    // Open Design launcher/taskbar icon: neutral surface-2 cell with a
    // 1px border and a monochrome stroke glyph (.la-ic / .tb-app).
    // Glyphs are authored in a 22×22 unit space and scaled to the cell.
    static void draw_generated_icon(int cx, int cy, int cw, int ch, int app_index, bool hover) {
        if (app_index < 0 || app_index > 18) return;

        int r = 8;                              // design radius (8px)
        uint32_t bg = VxTheme::SURFACE_2;       // cell background (surface-2)
        uint32_t cell_border = hover ? VxTheme::BORDER_STRONG_A : VxTheme::BORDER_ALPHA;
        vxui_draw_rounded_rect(cx, cy, cw, ch, r, bg);
        vxr_rounded_border(cx, cy, cw, ch, r, cell_border);

        // Glyph: centered, ~cw/2 wide (design: 22px glyph in 44px cell)
        int S = cw / 2;
        if (S < 8) S = 8;
        int u = S / 22; if (u < 1) u = 1;
        int ox = cx + (cw - S) / 2;
        int oy = cy + (ch - S) / 2;
        uint32_t col = hover ? VxTheme::TEXT_PRIMARY : VxTheme::FG_SOFT;
        uint32_t cyan = VxTheme::CYAN;

        auto g = [&](int x, int y, int w, int h, uint32_t c) {
            int rw = w * u; if (rw < 1) rw = 1;
            int rh = h * u; if (rh < 1) rh = 1;
            vxr_fill_rect(ox + x * u, oy + y * u, rw, rh, c);
        };
        auto gc = [&](int x, int y, int rad, uint32_t c) {
            int rr = rad * u; if (rr < 1) rr = 1;
            vxr_circle(ox + x * u, oy + y * u, rr, c);
        };

        switch (app_index) {
            case 0: // Calculator — display + keypad
                g(4, 4, 14, 4, col);
                g(4, 11, 4, 4, col); g(10, 11, 4, 4, col); g(16, 11, 4, 4, col);
                g(4, 17, 4, 4, col); g(10, 17, 4, 4, col); g(16, 17, 4, 4, col);
                break;
            case 1: // Notes — ruled lines
                g(4, 5, 14, 2, col); g(4, 9, 11, 2, col);
                g(4, 13, 14, 2, col); g(4, 17, 8, 2, col);
                break;
            case 2: // SysMon — pulse line (cyan accent per design)
                g(3, 16, 16, 2, col);
                g(6, 16, 2, -3, col); g(8, 13, 2, -4, col);
                g(10, 9, 2, 5, col); g(12, 14, 2, 2, col);
                g(14, 12, 2, -6, cyan); g(16, 6, 2, 10, cyan);
                break;
            case 3: // Files — folder
                g(5, 5, 7, 3, col); g(3, 8, 16, 11, col);
                break;
            case 4: // Settings — three sliders
                g(3, 5, 16, 2, col); gc(15, 6, 2, col);
                g(3, 11, 16, 2, col); gc(8, 12, 2, col);
                g(3, 17, 16, 2, col); gc(17, 18, 2, col);
                break;
            case 5: // Terminal — box + prompt
                g(4, 4, 14, 2, col); g(4, 4, 2, 13, col);
                g(16, 4, 2, 13, col); g(4, 16, 14, 2, col);
                g(7, 8, 3, 2, cyan); g(9, 10, 3, 2, cyan); g(7, 12, 3, 2, cyan);
                g(13, 14, 5, 2, col);
                break;
            case 6: // Snake — three rounded segments
                vxui_draw_rounded_rect(ox + 5*u, oy + 5*u, 4*u, 4*u, 2, col);
                vxui_draw_rounded_rect(ox + 11*u, oy + 5*u, 4*u, 4*u, 2, col);
                vxui_draw_rounded_rect(ox + 11*u, oy + 11*u, 4*u, 4*u, 2, col);
                vxui_draw_rounded_rect(ox + 5*u, oy + 11*u, 4*u, 4*u, 2, col);
                break;
            case 7: // Browser — globe
                gc(11, 11, 6, col);
                g(10, 3, 2, 16, col); g(3, 10, 16, 2, col);
                break;
            case 8: // Mail — envelope
                g(3, 5, 16, 12, col);
                g(4, 6, 2, 2, col); g(7, 8, 2, 2, col);
                g(10, 9, 2, 1, col); g(13, 8, 2, 2, col); g(16, 6, 2, 2, col);
                break;
            case 9: // Gallery — photo frame + sun + mountain
                g(3, 3, 16, 16, col);
                gc(7, 8, 2, col);
                g(3, 18, 16, 1, col); g(6, 14, 4, 2, col);
                g(8, 11, 3, 2, col); g(11, 16, 3, 2, col); g(14, 12, 3, 2, col);
                break;
            case 10: // Media — music note
                g(12, 4, 2, 12, col); g(12, 4, 6, 2, col);
                g(8, 12, 5, 4, col);
                break;
            case 11: // Clock — face + hands
                gc(11, 11, 8, col);
                g(10, 11, 2, 6, col); g(10, 10, 6, 2, col);
                break;
            case 12: // About — info badge
                gc(11, 11, 8, col);
                g(10, 6, 2, 2, col); g(10, 10, 2, 7, col);
                break;
            case 13: // Tasks — checklist
                g(4, 5, 10, 2, col); g(4, 11, 10, 2, col); g(4, 17, 10, 2, col);
                g(16, 4, 2, 4, col); g(17, 7, 2, 2, col); g(18, 9, 2, 2, col);
                g(16, 10, 2, 4, col); g(17, 13, 2, 2, col); g(18, 15, 2, 2, col);
                g(16, 16, 2, 4, col); g(17, 19, 2, 2, col); g(18, 21, 2, 2, col);
                break;
            case 14: // Editor — </> code brackets
                g(4, 4, 2, 14, col); g(6, 6, 2, 10, col);
                g(16, 4, 2, 14, col); g(14, 6, 2, 10, col);
                g(11, 3, 2, 16, cyan);
                break;
            case 15: // Docs — page with lines
                g(5, 3, 10, 2, col); g(5, 5, 12, 16, col);
                g(7, 9, 8, 2, col); g(7, 13, 8, 2, col); g(7, 17, 5, 2, col);
                break;
            case 16: // Archive — box
                g(3, 5, 16, 3, col); g(3, 8, 16, 10, col);
                g(3, 8, 16, 1, col); g(9, 8, 4, 4, col);
                break;
            case 17: // Software — store bag
                g(4, 7, 14, 12, col); g(3, 6, 16, 2, col);
                g(10, 4, 2, 3, col); g(7, 10, 3, 3, col);
                break;
            case 18: // Shot — camera
                g(4, 6, 14, 3, col); g(3, 9, 16, 10, col);
                gc(11, 13, 4, col); gc(11, 13, 2, VxTheme::SURFACE_2);
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
        const int r = VxTheme::RADIUS_MD;
        const int tb_h = VxTheme::TITLE_BAR_H;
        const uint32_t accent = VxTheme::accent();

        if (g_state.show_window_shadows) {
            vxr_shadow(w.x, w.y, w.w, w.h, 18);
        }

        uint32_t body_bg = VxTheme::SURFACE_1;
        vxui_draw_rounded_rect(w.x, w.y, w.w, w.h, r, body_bg);

        uint32_t border_color = w.focused ? 0xFF98E3EE : VxTheme::BORDER_ALPHA; // focused: mix(#00f0ff 30%,#d9dee7)
        vxr_rounded_border(w.x, w.y, w.w, w.h, r, border_color);

        if (w.focused) {
            vxr_rounded_border(w.x - 1, w.y - 1, w.w + 2, w.h + 2, r + 1, 0x6600F0FF); // 1px cyan ring
        }

        fill_rounded_top(w.x + 1, w.y + 1, w.w - 2, tb_h, body_bg);
        vxr_fill_rect(w.x + 1, w.y + tb_h, w.w - 2, 1, VxTheme::BORDER_ALPHA);

        const char* titles[] = {
            "Calculator","Notes","SysMon","Files","Settings","Terminal","Snake","Browser",
            "Mail","Gallery","Media","Clock","About","Tasks","Code Editor","Doc Viewer",
            "Archive Manager","Software Store","Screen Capture"
        };
        int title_idx = (int)w.app - 1;
        if (title_idx >= 0 && title_idx < 19) {
            const char* tn = titles[title_idx];
            int icon_x = w.x + 11;
            int icon_y = w.y + 6;
            draw_app_icon_in_cell(icon_x, icon_y, 16, 16, title_idx, false);

            int tx = w.x + 33;
            int ty = w.y + 9;
            uint32_t title_col = w.focused ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_MUTED;
            vx_text::draw(tx, w.y + 18, 14, tn, title_col, VxTheme::SURFACE_1);
        }

        const int btn_sz = 28;
        const int btn_gap = 2;
        const int btn_y = w.y;
        const int close_x = w.x + w.w - 8 - btn_sz;
        const int max_x = close_x - btn_gap - btn_sz;
        const int min_x = max_x - btn_gap - btn_sz;

        bool close_hover = (g_state.mouse_x >= close_x && g_state.mouse_x < close_x + btn_sz &&
                            g_state.mouse_y >= btn_y && g_state.mouse_y < btn_y + btn_sz);
        bool max_hover = (g_state.mouse_x >= max_x && g_state.mouse_x < max_x + btn_sz &&
                          g_state.mouse_y >= btn_y && g_state.mouse_y < btn_y + btn_sz);
        bool min_hover = (g_state.mouse_x >= min_x && g_state.mouse_x < min_x + btn_sz &&
                          g_state.mouse_y >= btn_y && g_state.mouse_y < btn_y + btn_sz);

        uint32_t btn_base = 0x00000000;
        uint32_t close_bg = close_hover ? 0xFF363636 : btn_base; // mix(#f7f8fa 16%, #111)
        uint32_t max_bg = max_hover ? VxTheme::SURFACE_2 : btn_base;
        uint32_t min_bg = min_hover ? VxTheme::SURFACE_2 : btn_base;

        vxui_draw_rounded_rect(min_x, btn_y, btn_sz, btn_sz, r, min_bg);
        vxui_draw_rounded_rect(max_x, btn_y, btn_sz, btn_sz, r, max_bg);
        vxui_draw_rounded_rect(close_x, btn_y, btn_sz, btn_sz, r, close_bg);

        uint32_t icon_col = w.focused ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
        if (min_hover) icon_col = VxTheme::TEXT_PRIMARY;
        if (max_hover) icon_col = VxTheme::TEXT_PRIMARY;
        if (close_hover) icon_col = 0xFFFFFFFF;

        vxr_fill_rect(min_x + 9, btn_y + 15, 10, 1, icon_col);
        vxr_fill_rect(max_x + 9, btn_y + 9, 10, 1, icon_col);
        vxr_fill_rect(max_x + 9, btn_y + 9, 1, 10, icon_col);
        vxr_fill_rect(max_x + 18, btn_y + 9, 1, 10, icon_col);
        vxr_fill_rect(max_x + 9, btn_y + 18, 10, 1, icon_col);
        for (int i = 0; i < 8; i++) {
            vxr_fill_rect(close_x + 10 + i, btn_y + 10 + i, 1, 1, icon_col);
            vxr_fill_rect(close_x + 17 - i, btn_y + 10 + i, 1, 1, icon_col);
        }

        VxClipRect old_clip = g_vxr_ctx.push_clip(w.x, w.y + tb_h, w.w, w.h - tb_h);

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
        } else if (w.app == VX_APP_CODE_EDITOR) {
            draw_app_code_editor(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_DOC_VIEWER) {
            draw_app_doc_viewer(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_ARCHIVE) {
            draw_app_archive_manager(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_STORE) {
            draw_app_software_center(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        } else if (w.app == VX_APP_SHOT) {
            draw_app_screenshot(w, g_frame, g_state.mouse_x, g_state.mouse_y, clicked);
        }

        g_vxr_ctx.pop_clip(old_clip);
    }

    // V5: lerp_color delegates to VXRender's color utility
    static uint32_t lerp_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t) {
        return VxColor::lerp(c1, c2, t, max_t);
    }

    // Alpha blend one pixel (src OVER dst) exactly matching the GOP backbuffer blend:
    //   r_out = (r_src*a + r_bg*(255-a)) / 255, output alpha forced to 0xFF.
    static inline uint32_t wp_blend(uint32_t dst, uint32_t color) {
        uint32_t a = (color >> 24) & 0xFF;
        if (a == 255) return color;
        if (a == 0) return dst;
        uint32_t rs = (color >> 16) & 0xFF, gs = (color >> 8) & 0xFF, bs = color & 0xFF;
        uint32_t rd = (dst >> 16) & 0xFF, gd = (dst >> 8) & 0xFF, bd = dst & 0xFF;
        uint32_t ro = (rs * a + rd * (255 - a)) / 255;
        uint32_t go = (gs * a + gd * (255 - a)) / 255;
        uint32_t bo = (bs * a + bd * (255 - a)) / 255;
        return (0xFFu << 24) | (ro << 16) | (go << 8) | bo;
    }

    // Fill a horizontal run in the cache (blended). Mirrors vxr_fill_rect clipping semantics.
    static void wp_row_fill(int x, int y, int w, uint32_t color) {
        if (y < 0 || y >= 768) return;
        uint32_t a = (color >> 24) & 0xFF;
        uint32_t* row = g_wp_cache + (uint32_t)y * (uint32_t)g_wp_w;
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px >= 0 && px < g_wp_w) row[px] = wp_blend(row[px], color);
        }
    }

    // Filled circle directly into the cache — same geometry as vxr_circle.
    static void wp_circle(int cx, int cy, int radius, uint32_t color) {
        if (radius < 0) return;
        for (int dy = -radius; dy <= radius; dy++) {
            int half = 0;
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) half = dx;
            }
            if (half >= 0) wp_row_fill(cx - half, cy + dy, half * 2 + 1, color);
        }
    }

    static void build_wallpaper_cache(uint32_t W, uint32_t H) {
        // Gradient (identical math to the runtime path in draw_polished_desktop)
        const uint32_t wp_top = 0xFF121E30;
        const uint32_t wp_mid = 0xFF111111;
        const uint32_t wp_br  = 0xFF12243E;
        for (uint32_t y = 0; y < H; y++) {
            uint32_t t100 = 25u + (50u * y) / H;
            if (t100 > 100) t100 = 100;
            uint32_t color;
            if (t100 <= 52) color = VxColor::lerp(wp_top, wp_mid, t100 * 100u / 52u, 100u);
            else color = VxColor::lerp(wp_mid, wp_br, (t100 - 52u) * 100u / 48u, 100u);
            wp_row_fill(0, (int)y, (int)W, color);
        }
        // Radial glows (same order as the draw path)
        if (g_state.show_desktop_glow) {
            const int gcx = (int)(W * 20u / 100u);
            const int gcy = (int)(H * 8u / 100u);
            const int gR = (int)(W * 90u / 100u);
            wp_circle(gcx, gcy, gR, VxColor::with_alpha(0x001677FF, 8));
            wp_circle(gcx, gcy, gR * 8 / 10, VxColor::with_alpha(0x001677FF, 12));
            wp_circle(gcx, gcy, gR * 55 / 100, VxColor::with_alpha(0x001677FF, 25));
            const int ccx = (int)(W * 86u / 100u);
            const int ccy = (int)(H * 98u / 100u);
            const int cR = (int)(W * 60u / 100u);
            wp_circle(ccx, ccy, cR, VxColor::with_alpha(0xFF00F0FF, 3));
            wp_circle(ccx, ccy, cR * 8 / 10, VxColor::with_alpha(0xFF00F0FF, 4));
            wp_circle(ccx, ccy, cR * 5 / 10, VxColor::with_alpha(0xFF00F0FF, 9));
        }
        // Faint 56px grid
        for (uint32_t gx = 0; gx < W; gx += 56) wp_row_fill((int)gx, 0, 1, 0x04D9DEE7);
        for (uint32_t gy = 0; gy < H; gy += 56) wp_row_fill(0, (int)gy, (int)W, 0x04D9DEE7);
    }

    static void draw_polished_desktop(uint32_t W, uint32_t H) {
        // ===== Workspace wallpaper (Open Design: 163deg tint over #111 + glows + grid) =====
        // Fast path: blit the cached wallpaper, drawn once by build_wallpaper_cache().
        if (!g_wp_cached) {
            if (g_wp_cache == nullptr) {
                g_wp_cache = (uint32_t*)vxair_kmalloc(W * H * 4);
                if (g_wp_cache) g_wp_w = (int)W;
            }
            if (g_wp_cache) {
                build_wallpaper_cache(W, H);
                g_wp_cached = true;
            }
        }
        if (g_wp_cached) {
            vxair_fb_blit(g_wp_cache, 0, 0, (int)W, (int)H);
        } else {
        const uint32_t wp_top = 0xFF121E30;  // mix(#1677ff 13%, #111111)
        const uint32_t wp_mid = 0xFF111111;  // midpoint
        const uint32_t wp_br  = 0xFF12243E;  // mix(#1677ff 19%, #111111)
        // Diagonal-ish: sample gradient at each row's midpoint along the 163deg axis
        for (uint32_t y = 0; y < H; y++) {
            uint32_t t100 = 25u + (50u * y) / H; // 0..100 along diagonal at row midpoint
            if (t100 > 100) t100 = 100;
            uint32_t color;
            if (t100 <= 52) {
                color = VxColor::lerp(wp_top, wp_mid, t100 * 100u / 52u, 100u);
            } else {
                color = VxColor::lerp(wp_mid, wp_br, (t100 - 52u) * 100u / 48u, 100u);
            }
            vxr_fill_rect(0, y, W, 1, color);
        }
        // Radial blue glow at 20%/8% (mix #1677ff 16% peak) — 3-stop fade
        if (g_state.show_desktop_glow) {
            const int gcx = (int)(W * 20u / 100u);
            const int gcy = (int)(H * 8u / 100u);
            const int gR = (int)(W * 90u / 100u);
            vxr_circle(gcx, gcy, gR, VxColor::with_alpha(0x001677FF, 8));   // outer soft halo
            vxr_circle(gcx, gcy, gR * 8 / 10, VxColor::with_alpha(0x001677FF, 12));
            vxr_circle(gcx, gcy, gR * 55 / 100, VxColor::with_alpha(0x001677FF, 25)); // core ~16%
            // Cyan glow at 86%/98% (mix #00f0ff 5% peak)
            const int ccx = (int)(W * 86u / 100u);
            const int ccy = (int)(H * 98u / 100u);
            const int cR = (int)(W * 60u / 100u);
            vxr_circle(ccx, ccy, cR, VxColor::with_alpha(0xFF00F0FF, 3));
            vxr_circle(ccx, ccy, cR * 8 / 10, VxColor::with_alpha(0xFF00F0FF, 4));
            vxr_circle(ccx, ccy, cR * 5 / 10, VxColor::with_alpha(0xFF00F0FF, 9));
        }
        // Faint 56px grid (mix #d9dee7 1.8%, transparent)
        for (uint32_t gx = 0; gx < W; gx += 56) vxr_fill_rect(gx, 0, 1, H, 0x04D9DEE7);
        for (uint32_t gy = 0; gy < H; gy += 56) vxr_fill_rect(0, gy, W, 1, 0x04D9DEE7);
        } // else: fallback when wallpaper cache is unavailable

        // Draw windows on stage
        for (int z = 0; z < 16; z++) {
            int i = g_z_order[z];
            if (g_state.windows[i].open && !g_state.windows[i].is_minimized) {
                draw_window(g_state.windows[i], g_window_clicked[i]);
            }
        }

        if (g_state.focus_dim) {
            for (int i = 0; i < 16; i++) {
                VxWindow& ww = g_state.windows[i];
                if (ww.open && !ww.focused) {
                    vxr_fill_rect(ww.x, ww.y, ww.w, ww.h, 0x66000000);
                }
            }
        }

        // ===== FULL-WIDTH BOTTOM TASKBAR (56px) =====
        uint32_t dock_h = VxTheme::TASKBAR_H;
        uint32_t dock_y = H - dock_h;
        vxr_fill_rect(0, dock_y, W, dock_h, VxTheme::BG);
        vxr_fill_rect(0, dock_y, W, 1, 0x6600F0FF); // 1px translucent cyan top border

        // --- ZONE 1: LEFT (Launcher & Apps) ---
        uint32_t lx = VxTheme::TASKBAR_PAD;
        uint32_t ly = dock_y + 8;
        bool launcher_hover = (g_state.mouse_x >= (int)lx && g_state.mouse_x <= (int)lx + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
        // Open Design: launcher button (.tb-start) always carries a surface-2 background
        vxui_draw_rounded_rect(lx, ly, 40, 40, 8, launcher_hover ? VxTheme::SURFACE_3 : VxTheme::SURFACE_2);
        uint32_t cyan = VxTheme::CYAN;
        vxr_fill_rect(lx + 13, ly + 13, 6, 6, cyan);
        vxr_fill_rect(lx + 21, ly + 13, 6, 6, cyan);
        vxr_fill_rect(lx + 13, ly + 21, 6, 6, cyan);
        vxr_fill_rect(lx + 21, ly + 21, 6, 6, cyan);
        // Separator after launcher (tb-sep)
        vxr_fill_rect(lx + 48, dock_y + 17, 1, 22, 0x6BD9DEE7);

        const VxAppId task_apps[6] = {
            VX_APP_FILES, VX_APP_TERMINAL, VX_APP_BROWSER,
            VX_APP_CODE_EDITOR, VX_APP_MAIL, VX_APP_MEDIA_PLAYER
        };
        uint32_t tx_base = lx + 60;
        for (int i = 0; i < 6; i++) {
            int win_idx = find_taskbar_window_idx(task_apps[i]);
            bool is_open = (win_idx != -1);
            bool is_focused = is_open && g_state.windows[win_idx].focused;
            bool ihover = (g_state.mouse_x >= (int)tx_base && g_state.mouse_x <= (int)tx_base + 40 && g_state.mouse_y >= (int)ly && g_state.mouse_y <= (int)ly + 40);
            uint32_t item_bg = (is_open || ihover) ? VxTheme::SURFACE_2 : 0x00000000;
            if (item_bg != 0) {
                vxui_draw_rounded_rect(tx_base, ly, 40, 40, 8, item_bg);
            }
            // Running indicator: 16x2 bottom bar (dim cyan / active cyan)
            if (is_focused) {
                vxr_fill_rect(tx_base + 12, ly + 38, 16, 2, cyan);
            } else if (is_open) {
                vxr_fill_rect(tx_base + 12, ly + 38, 16, 2, 0x6600F0FF);
            }
            draw_app_icon_in_cell(tx_base + 4, ly + 4, 32, 32, task_apps[i] - 1, ihover);
            tx_base += 48;
        }

        // --- ZONE 2: CENTER (Workspaces) ---
        uint32_t cx = (W - 36) / 2;
        uint32_t cy = dock_y + 26;
        vxui_draw_rounded_rect(cx, cy, 14, 4, 2, cyan); // active pill
        vxr_circle(cx + 24, cy + 2, 2, VxTheme::MUTED_DIM);
        vxr_circle(cx + 32, cy + 2, 2, VxTheme::MUTED_DIM);

        // --- ZONE 3: RIGHT (Tray & Clock) ---
        // Separator
        vxr_fill_rect(W - 12 - 168, dock_y + 17, 1, 22, 0x6BD9DEE7);
        uint32_t rx = W - 12 - 152;
        uint32_t ry = dock_y + 8;
        // Tray wifi button hover (design: .tb-app.tb-tray:hover → surface-2)
        bool tray_hover = (g_state.mouse_x >= (int)rx - 8 && g_state.mouse_x <= (int)rx + 40 && g_state.mouse_y >= (int)ry && g_state.mouse_y <= (int)ry + 40);
        if (tray_hover) {
            vxui_draw_rounded_rect(rx - 8, ry - 2, 48, 44, 8, VxTheme::SURFACE_2);
        }
        // Wi-Fi (cyan arcs)
        vxr_fill_rect(rx + 6, ry + 26, 6, 2, cyan);
        vxr_fill_rect(rx + 7, ry + 22, 4, 1, cyan);
        vxr_fill_rect(rx + 8, ry + 18, 2, 1, cyan);
        vxr_fill_rect(rx + 9, ry + 30, 2, 1, 0x00F0FF);
        // Bluetooth (cyan "B" glyph)
        vxr_fill_rect(rx + 24, ry + 14, 1, 20, cyan);
        vxr_fill_rect(rx + 24, ry + 14, 6, 6, cyan);
        vxr_fill_rect(rx + 24, ry + 21, 6, 6, cyan);
        // Volume (speaker + waves)
        vxr_fill_rect(rx + 44, ry + 15, 2, 15, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 44, ry + 15, 6, 6, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 44, ry + 24, 6, 6, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 52, ry + 19, 1, 7, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 53, ry + 17, 1, 11, VxTheme::FG_SOFT);
        // Battery (outline + fill)
        vxr_rounded_rect(rx + 64, ry + 14, 18, 16, 3, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 83, ry + 19, 2, 6, VxTheme::FG_SOFT);
        vxr_fill_rect(rx + 66, ry + 16, 12, 12, VxTheme::SUCCESS);

        uint32_t clock_x = W - 12 - 50;
        uint32_t clock_y_text = dock_y + 14;
        const char* clock_str = "14:30";
        vx_text::draw(clock_x, dock_y + 26, 16, clock_str, VxTheme::FG, VxTheme::BG);
        const char* date_str = "Aug 3";
        vx_text::draw(clock_x, dock_y + 41, 12, date_str, VxTheme::MUTED, VxTheme::BG);

        // Launcher Overlay
        if (g_state.launcher_open) {
            VxLauncherLayout L = compute_launcher_layout(W, H, g_state.launcher_search, g_state.launcher_search_len);
            vxui_draw_rounded_rect(L.card.x, L.card.y, L.card.w, L.card.h, 8, VxTheme::SURFACE_1);
            vxr_rounded_border(L.card.x, L.card.y, L.card.w, L.card.h, 8, VxTheme::BORDER_STRONG_A);
            vxr_rounded_border(L.card.x - 1, L.card.y - 1, L.card.w + 2, L.card.h + 2, 9, 0x2900F0FF); // design cyan ring

            // Search field
            int sx = L.card.x + 14;
            int sy = L.card.y + 14;
            int sw = L.card.w - 28;
            int sh = 44;
            vxui_draw_rounded_rect(sx, sy, sw, sh, 8, VxTheme::SURFACE_2);
            vxr_rounded_border(sx, sy, sw, sh, 8, VxTheme::BORDER_ALPHA);
            vxr_circle(sx + 16, sy + 22, 6, VxTheme::MUTED);
            vxr_fill_rect(sx + 21, sy + 27, 6, 2, VxTheme::MUTED);
            const char* search_txt = (g_state.launcher_search_len > 0) ? g_state.launcher_search : "Search apps...";
            uint32_t search_col = (g_state.launcher_search_len > 0) ? VxTheme::FG : VxTheme::MUTED;
            vx_text::draw(sx + 36, sy + 29, 14, search_txt, search_col, VxTheme::SURFACE_2);
            if (g_state.launcher_search_len > 0) {
                int caret_x = sx + 36 + vx_text::text_width(14, g_state.launcher_search);
                vxr_fill_rect(caret_x, sy + 12, 2, 22, VxTheme::accent());
            }

            for (int i = 0; i < L.item_count; i++) {
                int ax = L.items[i].x;
                int ay = L.items[i].y;
                bool ihover = L.items[i].contains(g_state.mouse_x, g_state.mouse_y);
                if (ihover) {
                    vxui_draw_rounded_rect(ax, ay, L.items[i].w, L.items[i].h, 8, VxTheme::SURFACE_2);
                }
                draw_app_icon_in_cell(L.icon_cells[i].x, L.icon_cells[i].y, L.icon_cells[i].w, L.icon_cells[i].h, g_app_ids[L.app_indices[i]] - 1, ihover);

                const char* name = g_app_names[L.app_indices[i]];
                int name_w = vx_text::text_width(12, name);
                int name_x = ax + (L.items[i].w - name_w) / 2;
                vx_text::draw(name_x, ay + 78, 12, name, VxTheme::FG_SOFT, ihover ? VxTheme::SURFACE_2 : VxTheme::SURFACE_1);
            }

            // Footer / identity row
            int footer_y = L.card.y + L.card.h - 58 - 14;
            vxr_fill_rect(L.card.x + 14, footer_y, L.card.w - 28, 1, VxTheme::BORDER_ALPHA);
            vxr_circle(L.card.x + 34, footer_y + 18, 13, VxTheme::SURFACE_2);
            draw_abstract_char(L.card.x + 29, footer_y + 12, 'e', VxTheme::FG);
            vx_text::draw(L.card.x + 56, footer_y + 24, 13, "ethan", VxTheme::FG, VxTheme::SURFACE_1);
            vx_text::draw(L.card.x + 56, footer_y + 38, 11, "vextryn@air", VxTheme::MUTED, VxTheme::SURFACE_1);
            int power_x = L.card.x + L.card.w - 120;
            for (int i = 0; i < 3; i++) {
                vxui_draw_rounded_rect(power_x + i * 38, footer_y + 8, 30, 30, 8, VxTheme::SURFACE_2);
                vxr_rounded_border(power_x + i * 38, footer_y + 8, 30, 30, 8, VxTheme::BORDER_ALPHA);
            }
            draw_abstract_char(power_x + 10, footer_y + 17, 'L', 0xFFF7F8FA);
            draw_abstract_char(power_x + 48, footer_y + 17, 'R', 0xFFF7F8FA);
            draw_abstract_char(power_x + 86, footer_y + 17, 'S', 0xFFF7F8FA);
        }

        // Control Center overlay (drawn after the launcher, before the cursor)
        if (g_state.control_center_open) {
            draw_control_center(W, H, g_state.mouse_x, g_state.mouse_y, false);
        }

        // Cursor
        int max_ptr_x = (int)W - 24;
        int max_ptr_y = (int)H - 36;
        if (max_ptr_x < 0) max_ptr_x = 0;
        if (max_ptr_y < 0) max_ptr_y = 0;
        uint32_t ptr_x = (g_state.mouse_x < 0) ? 0 : (g_state.mouse_x > max_ptr_x ? (uint32_t)max_ptr_x : (uint32_t)g_state.mouse_x);
        uint32_t ptr_y = (g_state.mouse_y < 0) ? 0 : (g_state.mouse_y > max_ptr_y ? (uint32_t)max_ptr_y : (uint32_t)g_state.mouse_y);
        if (g_state.large_cursor) {
            for (int i = 0; i < 18; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 2, 2, 0x88000000);
            vxr_fill_rect(ptr_x + 2, ptr_y + 4, 2, 30, 0x88000000);
            for (int i = 0; i < 18; i++) vxr_fill_rect(ptr_x + i, ptr_y + i, 2, 2, 0xFF000000);
            vxr_fill_rect(ptr_x, ptr_y, 2, 30, 0xFF000000);
            for (int i = 0; i < 24; i++) vxr_fill_rect(ptr_x + 2, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            for (int i = 1; i < 14; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 2 + i, 2, 2, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 2, ptr_y + 26, 6, 2, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 6, ptr_y + 26, 2, 6, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x, ptr_y, 4, 4, VxTheme::accent());
        } else {
            vxr_fill_rect(ptr_x + 2, ptr_y + 3, 1, 18, 0x88000000);
            for (int i = 0; i < 12; i++) vxr_fill_rect(ptr_x + 2 + i, ptr_y + 3 + i, 1, 1, 0x88000000);
            vxr_fill_rect(ptr_x - 1, ptr_y - 1, 1, 18, 0xFF000000);
            for (int i = 0; i < 12; i++) vxr_fill_rect(ptr_x - 1 + i, ptr_y - 1 + i, 1, 1, 0xFF000000);
            for (int i = 0; i < 16; i++) vxr_fill_rect(ptr_x, ptr_y + i, 1, 1, 0xFFFFFFFF);
            for (int i = 1; i < 10; i++) vxr_fill_rect(ptr_x + i, ptr_y + i, 1, 1, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 1, ptr_y + 10, 5, 1, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 5, ptr_y + 11, 2, 5, 0xFFFFFFFF);
            vxr_fill_rect(ptr_x + 7, ptr_y + 9, 3, 6, 0xFFFFFFFF);
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
        g_state.wifi_enabled = true;      // Open Design: WiFi switch is ON by default
        g_state.bluetooth_enabled = true; // Open Design: Bluetooth switch is ON by default
        g_state.airdrop_enabled = false;
        g_state.dnd_enabled = false;
        
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

        g_state.windows[0]  = {false, VX_APP_CALCULATOR,   160, 130, 300, 390, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[1]  = {false, VX_APP_NOTES,        395, 110, 420, 420, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[2]  = {false, VX_APP_SYSMON,       235, 170, 500, 330, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[3]  = {false, VX_APP_FILES,         36,  96, 420, 560, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[4]  = {false, VX_APP_SETTINGS,     500,  96, 488, 560, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[5]  = {false, VX_APP_TERMINAL,     50,  50,  600, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[6]  = {false, VX_APP_SNAKE,        200, 200, 400, 428, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[7]  = {false, VX_APP_BROWSER,      80,  80,  640, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[8]  = {false, VX_APP_MAIL,         120, 90,  720, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[9]  = {false, VX_APP_GALLERY,      300, 100, 480, 380, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[10] = {false, VX_APP_MEDIA_PLAYER, 180, 120, 400, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[11] = {false, VX_APP_CLOCK,        250, 80,  320, 340, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[12] = {false, VX_APP_ABOUT,        200, 100, 480, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[13] = {false, VX_APP_TASKS,        300, 140, 360, 400, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[14] = {false, VX_APP_CODE_EDITOR,  100, 80,  640, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[15] = {false, VX_APP_DOC_VIEWER,   140, 100, 600, 460, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[16] = {false, VX_APP_ARCHIVE,      180, 120, 520, 380, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[17] = {false, VX_APP_STORE,        120, 90,  660, 480, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[18] = {false, VX_APP_SHOT,         220, 140, 420, 300, false, 0, 0, false, false, false, 0,0,0,0};
        g_state.windows[19] = {false, VX_APP_NONE,         0,   0,   0,   0,   false, 0, 0, false, false, false, 0,0,0,0};
        
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
        VxDamageTracker damage_tracker;
        damage_tracker.reset();

        while (1) {
            handle_input(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 3: immediately before first desktop render");
            draw_polished_desktop(W, H);
            if (g_frame == 0) vxair_log_info("COMP MARK 4: immediately after first desktop render");

            // Sub-region dirty rect presentation step
            if (g_frame == 0 || damage_tracker.count == 0) {
                vxair_fb_flip();
            } else {
                vxair_gpu_fb_present_rects(damage_tracker.rects, damage_tracker.count);
                damage_tracker.reset();
            }

            if (g_frame == 0) vxair_log_info("COMP MARK 5: immediately after first framebuffer flip/present");
            vxair_hpet_sleep_ms(3);
            g_frame++;
            if (g_frame == 60) vxair_log_info("COMPOSITOR RUNNING (one-time log; periodic logs disabled to avoid serial throttling)");
        }
    }
}
