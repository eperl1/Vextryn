#pragma once

#include <stdint.h>

// Documented single-browser limitation: static state means only one browser instance can run correctly.
static char url_buffer[128] = {0};
static int url_len = 0;
static int caret_pos = 0;
static int current_page = 0; // 0=about:home, 1=about:blank, 2=network not implemented
static bool url_focused = false;

struct BrowserHistoryEntry {
    int page;
    char url[128];
    int url_len;
};

#define BROWSER_HISTORY_MAX 32
static BrowserHistoryEntry browser_history[BROWSER_HISTORY_MAX] = { {0, {0}, 0} };
static int history_count = 1;
static int history_idx = 0;

static void navigate_to(int page) {
    history_count = history_idx + 1;
    if (history_count < BROWSER_HISTORY_MAX) {
        browser_history[history_count].page = page;
        browser_history[history_count].url_len = url_len;
        for(int i=0; i<128; i++) browser_history[history_count].url[i] = url_buffer[i];
        history_idx++;
        history_count++;
    } else {
        for(int i=1; i<BROWSER_HISTORY_MAX; i++) {
            browser_history[i-1] = browser_history[i];
        }
        browser_history[BROWSER_HISTORY_MAX-1].page = page;
        browser_history[BROWSER_HISTORY_MAX-1].url_len = url_len;
        for(int i=0; i<128; i++) browser_history[BROWSER_HISTORY_MAX-1].url[i] = url_buffer[i];
    }
    current_page = page;
    caret_pos = url_len;
}

static void restore_history_idx() {
    current_page = browser_history[history_idx].page;
    url_len = browser_history[history_idx].url_len;
    for(int i=0; i<128; i++) url_buffer[i] = browser_history[history_idx].url[i];
    caret_pos = url_len;
}

static void browser_handle_key(char c) {
    if (c == 27) {
        url_focused = false;
    } else if (url_focused) {
        if (c == 17) {
            if (caret_pos > 0) caret_pos--;
        } else if (c == 18) {
            if (caret_pos < url_len) caret_pos++;
        } else if (c == '\b') {
            if (caret_pos > 0) {
                for (int i = caret_pos - 1; i < url_len - 1; i++) {
                    url_buffer[i] = url_buffer[i + 1];
                }
                url_len--;
                caret_pos--;
                url_buffer[url_len] = 0;
            }
        } else if (c == '\n') {
            auto check = [](const char* target, int len) {
                if (url_len != len) return false;
                for(int k=0; k<len; k++) if(url_buffer[k] != target[k]) return false;
                return true;
            };
            if (check("about:home", 10)) navigate_to(0);
            else if (check("about:blank", 11)) navigate_to(1);
            else if (check("about:version", 13)) navigate_to(3);
            else if (check("about:help", 10)) navigate_to(4);
            else navigate_to(2);
            url_focused = false;
        } else {
            if (c >= 32 && c <= 126) {
                if (url_len < 127) {
                    for (int i = url_len; i > caret_pos; i--) {
                        url_buffer[i] = url_buffer[i - 1];
                    }
                    url_buffer[caret_pos] = c;
                    url_len++;
                    caret_pos++;
                    url_buffer[url_len] = 0;
                }
            }
        }
    }
}

void draw_app_browser(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    int start_x = w.x + 2;
    int start_y = w.y + 28;
    int width = w.w - 4;
    int height = w.h - 30;

    if (width <= 0 || height <= 0) return;

    auto fill = [](int x, int y, int w, int h, uint32_t c) {
        if (w > 0 && h > 0) {
            vxair_fb_fill_rect(x, y, (uint32_t)w, (uint32_t)h, c);
        }
    };

    
    int tab_h = 26;
    int toolbar_h = 36;
    if (height < tab_h + toolbar_h) return; // Too small

    // 1. Tab strip background
    fill(start_x, start_y, width, tab_h, 0xFF282C34);

    // Active tab
    int tab_w = 160;
    if (tab_w > width - 10) tab_w = width - 10;
    fill(start_x + 8, start_y + 4, tab_w, tab_h - 4, 0xFF3E4451);
    // Tab geometry icon and text
    fill(start_x + 16, start_y + 12, 12, 12, 0xFF61AFEF); // Favicon shape
    fill(start_x + 36, start_y + 16, tab_w - 50, 4, 0xFFABB2BF); // Tab text shape

    // 2. Toolbar background
    int toolbar_y = start_y + tab_h;
    fill(start_x, toolbar_y, width, toolbar_h, 0xFF3E4451);

    // 3. Toolbar buttons (Back, Forward, Reload)
    int btn_w = 28;
    int btn_h = 28;
    int btn_y = toolbar_y + 4;
    
    auto draw_btn = [&](int bx, uint32_t symbol_col) {
        bool hover = (mouse_x >= bx && mouse_x < bx + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);
        uint32_t bg = hover ? (clicked ? 0xFF282C34 : 0xFF4B5363) : 0xFF3E4451;
        fill(bx, btn_y, btn_w, btn_h, bg);
        // Simple inner geometry for button symbol
        fill(bx + 8, btn_y + 12, btn_w - 16, 4, symbol_col);
    };

    int back_x = start_x + 8;
    int fwd_x = back_x + btn_w + 4;
    int rel_x = fwd_x + btn_w + 8;

    draw_btn(back_x, (history_idx > 0) ? 0xFFABB2BF : 0xFF5C6370); // Back
    if (clicked && mouse_x >= back_x && mouse_x < back_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
        if (history_idx > 0) {
            history_idx--;
            restore_history_idx();
        }
    }
    
    draw_btn(fwd_x, (history_idx < history_count - 1) ? 0xFFABB2BF : 0xFF5C6370);  // Forward
    if (clicked && mouse_x >= fwd_x && mouse_x < fwd_x + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
        if (history_idx < history_count - 1) {
            history_idx++;
            restore_history_idx();
        }
    }
    
    draw_btn(rel_x, 0xFFABB2BF);  // Reload

    // 4. Address bar
    int addr_x = rel_x + btn_w + 12;
    int addr_w = width - (addr_x - start_x) - 12;
    if (addr_w > 0) {
        bool addr_hover = (mouse_x >= addr_x && mouse_x < addr_x + addr_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);
        int text_x = addr_x + 24;
        if (clicked) {
            url_focused = addr_hover;
            if (addr_hover) {
                int clicked_char = (mouse_x - text_x + 4) / 8;
                if (clicked_char < 0) clicked_char = 0;
                if (clicked_char > url_len) clicked_char = url_len;
                caret_pos = clicked_char;
            }
        }
        fill(addr_x, btn_y, addr_w, btn_h, addr_hover ? 0xFF1E2229 : 0xFF21252B);
        // Lock icon
        fill(addr_x + 8, btn_y + 10, 8, 8, 0xFF98C379);
        // Address text
        int text_y = btn_y + 6;
        for (int i = 0; i < url_len; i++) {
            draw_abstract_char(text_x + i * 8, text_y, url_buffer[i], 0xFFABB2BF);
        }
        if (w.focused && url_focused && (frame % 60 < 30)) {
            fill(text_x + caret_pos * 8, text_y, 2, 16, 0xFFABB2BF);
        }
    }

    // 5. Content pane (about:home)
    int content_y = toolbar_y + toolbar_h;
    int content_h = height - tab_h - toolbar_h;
    if (content_h <= 0) return;

    fill(start_x, content_y, width, content_h, 0xFFFFFFFF);

    if (current_page == 1) {
        // about:blank
        fill(start_x, content_y, width, content_h, 0xFFFFFFFF);
    } else if (current_page == 2) {
        // Page not found
        fill(start_x, content_y, width, content_h, 0xFFFFFFFF);
        const char* msg = "Page not found";
        for (int i=0; msg[i] != 0; i++) {
            draw_abstract_char(start_x + 50 + i * 10, content_y + 50, msg[i], 0xFF000000);
        }
    } else if (current_page == 3) {
        // about:version
        fill(start_x, content_y, width, content_h, 0xFFFFFFFF);
        const char* msg = "Vextryn Browser v1.0";
        for (int i=0; msg[i] != 0; i++) {
            draw_abstract_char(start_x + 50 + i * 10, content_y + 50, msg[i], 0xFF000000);
        }
    } else if (current_page == 4) {
        // about:help
        fill(start_x, content_y, width, content_h, 0xFFFFFFFF);
        const char* msg = "Help: Type URL to navigate";
        for (int i=0; msg[i] != 0; i++) {
            draw_abstract_char(start_x + 50 + i * 10, content_y + 50, msg[i], 0xFF000000);
        }
    } else {
        fill(start_x, content_y, width, content_h, 0xFFF0F2F5);
        // Hero panel
    int hero_h = 120;
    if (hero_h > content_h / 2) hero_h = content_h / 2;
    int hero_w = width - 80;
    int hero_x = start_x + 40;
    int hero_y = content_y + 40;
    if (hero_w > 0 && hero_h > 0) {
        fill(hero_x, hero_y, hero_w, hero_h, 0xFFFFFFFF);
        // Hero logo
        fill(hero_x + (hero_w - 40) / 2, hero_y + 20, 40, 40, 0xFF61AFEF);
        
        // Search box inside hero
        int search_w = hero_w * 2 / 3;
        int search_h = 32;
        int search_x = hero_x + (hero_w - search_w) / 2;
        int search_y = hero_y + 70;
        if (search_w > 0 && search_h > 0 && search_y + search_h <= hero_y + hero_h) {
            bool search_hover = (mouse_x >= search_x && mouse_x < search_x + search_w && mouse_y >= search_y && mouse_y < search_y + search_h);
            fill(search_x, search_y, search_w, search_h, search_hover ? 0xFFF8F9FA : 0xFFE4E6EB);
            // Search icon
            fill(search_x + 12, search_y + 12, 8, 8, 0xFFABB2BF);
            // Search placeholder text
            fill(search_x + 28, search_y + 14, search_w / 3, 4, 0xFFC8CDD4);
        }
    }

    // Site card grid
    int grid_y = hero_y + hero_h + 30;
    int card_w = 90;
    int card_h = 70;
    int gap = 20;
    int cols = (width - 80 + gap) / (card_w + gap);
    if (cols > 5) cols = 5;
    
    if (cols > 0 && grid_y + card_h < content_y + content_h) {
        int grid_start_x = start_x + (width - (cols * card_w + (cols - 1) * gap)) / 2;
        for (int i = 0; i < cols; i++) {
            int cx = grid_start_x + i * (card_w + gap);
            bool hover = (mouse_x >= cx && mouse_x < cx + card_w && mouse_y >= grid_y && mouse_y < grid_y + card_h);
            fill(cx, grid_y, card_w, card_h, hover ? 0xFFE4E6EB : 0xFFFFFFFF);

            // Card icon/placeholder
            fill(cx + (card_w - 36) / 2, grid_y + 16, 36, 24, 0xFFD1D5DB);
            // Card text
            fill(cx + (card_w - 50) / 2, grid_y + 50, 50, 4, 0xFF9CA3AF);
        }
    }
    } // end else
}
