#pragma once

#include "../../vxui/vxui_advanced.hpp"
#include "../vxair_textinput.hpp"

struct BrowserHistoryEntry {
    int page;
    char url[128];
    int url_len;
};

#define BROWSER_HISTORY_MAX 32

struct BrowserTab {
    char url_buffer[128];
    int url_len;
    VxTextInput url_input;
    BrowserHistoryEntry history[BROWSER_HISTORY_MAX];
    int history_count;
    int history_idx;
    int current_page;
    char search_buffer[128];
    int search_len;
    VxTextInput search_input;
};

#define MAX_BROWSER_TABS 8
static BrowserTab browser_tabs[MAX_BROWSER_TABS];
static int num_browser_tabs = 0;
static int active_browser_tab = 0;

static uint64_t last_click_frame = 1000000;
static bool url_focused = false;
static bool search_focused = false;

static void create_browser_tab() {
    if (num_browser_tabs >= MAX_BROWSER_TABS) return;
    int idx = num_browser_tabs++;
    BrowserTab& tab = browser_tabs[idx];
    tab.url_len = 0;
    tab.url_buffer[0] = 0;
    tab.url_input.init(tab.url_buffer, &tab.url_len, 128);
    tab.history_count = 1;
    tab.history_idx = 0;
    tab.history[0] = {0, {0}, 0};
    tab.current_page = 0;
    tab.search_len = 0;
    tab.search_buffer[0] = 0;
    tab.search_input.init(tab.search_buffer, &tab.search_len, 128);
    active_browser_tab = idx;
    url_focused = false;
    search_focused = false;
}

static void restore_history_idx() {
    BrowserTab& tab = browser_tabs[active_browser_tab];
    tab.current_page = tab.history[tab.history_idx].page;
    tab.url_len = tab.history[tab.history_idx].url_len;
    for(int i=0; i<128; i++) tab.url_buffer[i] = tab.history[tab.history_idx].url[i];
}

static void browser_handle_key(char c) {
    BrowserTab& tab = browser_tabs[active_browser_tab];
    if (url_focused) {
        if (c == '\n' || c == '\r') {
            tab.current_page = 1;
            if (tab.history_count < BROWSER_HISTORY_MAX) {
                tab.history_idx = tab.history_count++;
                tab.history[tab.history_idx].page = 1;
                tab.history[tab.history_idx].url_len = tab.url_len;
                for(int i=0; i<128; i++) tab.history[tab.history_idx].url[i] = tab.url_buffer[i];
            }
        } else {
            tab.url_input.handle_key(c);
        }
    } else if (search_focused && tab.current_page == 0) {
        if (c == '\n' || c == '\r') {
            // Copy search to URL
            tab.url_len = 0;
            const char* prefix = "https://search.vextryn.com/?q=";
            for(int i=0; prefix[i]; i++) tab.url_buffer[tab.url_len++] = prefix[i];
            for(int i=0; i<tab.search_len; i++) {
                if (tab.url_len < 127) tab.url_buffer[tab.url_len++] = tab.search_buffer[i];
            }
            tab.url_buffer[tab.url_len] = 0;
            
            tab.current_page = 1;
            if (tab.history_count < BROWSER_HISTORY_MAX) {
                tab.history_idx = tab.history_count++;
                tab.history[tab.history_idx].page = 1;
                tab.history[tab.history_idx].url_len = tab.url_len;
                for(int i=0; i<128; i++) tab.history[tab.history_idx].url[i] = tab.url_buffer[i];
            }
        } else {
            tab.search_input.handle_key(c);
        }
    }
}

static void draw_app_browser(VxWindow& w, uint64_t frame, int mouse_x, int mouse_y, bool clicked) {
    if (num_browser_tabs == 0) {
        create_browser_tab();
    }
    BrowserTab& tab = browser_tabs[active_browser_tab];
    
    int start_x = w.x;
    int start_y = w.y + 28;
    int width = w.w;
    int height = w.h - 28;

    // Tabs area
    int tab_h = 36;
    vxr_fill_rect(start_x, start_y, width, tab_h, VxTheme::BASE_DEEP);
    
    int tab_w = 160;
    int tx = start_x + 10;
    for (int i = 0; i < num_browser_tabs; i++) {
        bool is_active = (i == active_browser_tab);
        bool thover = (mouse_x >= tx && mouse_x < tx + tab_w && mouse_y >= start_y && mouse_y < start_y + tab_h);
        
        bool close_hover = (mouse_x >= tx + tab_w - 24 && mouse_x < tx + tab_w - 4 && mouse_y >= start_y + 14 && mouse_y < start_y + 34);
        
        if (clicked && close_hover) {
            for (int j = i; j < num_browser_tabs - 1; j++) {
                browser_tabs[j] = browser_tabs[j+1];
            }
            num_browser_tabs--;
            if (active_browser_tab >= num_browser_tabs) {
                active_browser_tab = num_browser_tabs - 1;
            }
            if (active_browser_tab < 0) active_browser_tab = 0;
            clicked = false;
            i--;
            continue;
        } else if (clicked && thover) {
            active_browser_tab = i;
            url_focused = false;
            search_focused = false;
        }
        
        uint32_t bg = is_active ? VxTheme::SURFACE : (thover ? VxTheme::SURFACE_HIGH : VxTheme::BASE_DEEP);
        vxui_draw_rounded_rect(tx, start_y + 8, tab_w, tab_h - 8, VxTheme::RADIUS_MD, bg);
        
        // Favicon
        vxr_fill_rect(tx + 10, start_y + 18, 12, 12, VxTheme::accent());
        
        // Tab text
        const char* tab_title = browser_tabs[i].current_page == 0 ? "New Tab" : "Vextryn Web";
        for (int c = 0; tab_title[c]; c++) {
            draw_abstract_char(tx + 30 + c*8, start_y + 20, tab_title[c], VxTheme::TEXT_PRIMARY);
        }
        
        // Close button
        if (thover || is_active) {
            vxui_draw_rounded_rect(tx + tab_w - 24, start_y + 14, 20, 20, VxTheme::RADIUS_SM, close_hover ? VxTheme::accent_soft() : bg);
            draw_abstract_char(tx + tab_w - 18, start_y + 20, 'X', VxTheme::TEXT_MUTED);
        }
        
        tx += tab_w + 4;
    }
    
    // New Tab button (+)
    if (num_browser_tabs < MAX_BROWSER_TABS) {
        bool phover = (mouse_x >= tx && mouse_x < tx + 24 && mouse_y >= start_y + 12 && mouse_y < start_y + 36);
        if (clicked && phover) {
            create_browser_tab();
        }
        vxui_draw_rounded_rect(tx, start_y + 12, 24, 24, VxTheme::RADIUS_SM, phover ? VxTheme::SURFACE_HIGH : VxTheme::BASE_DEEP);
        draw_abstract_char(tx + 8, start_y + 18, '+', VxTheme::TEXT_PRIMARY);
    }

    // Toolbar area
    int toolbar_h = 44;
    int toolbar_y = start_y + tab_h;
    vxr_fill_rect(start_x, toolbar_y, width, toolbar_h, VxTheme::SURFACE);
    vxr_fill_rect(start_x, toolbar_y + toolbar_h - 1, width, 1, VxTheme::BORDER_SUBTLE);

    // Buttons
    int btn_w = 32;
    int btn_h = 32;
    int btn_y = toolbar_y + 6;

    auto draw_btn = [&](int bx, const char* symbol, bool active) {
        bool hover = (mouse_x >= bx && mouse_x < bx + btn_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);
        uint32_t bg = hover ? VxTheme::OVERLAY : VxTheme::SURFACE;
        vxui_draw_rounded_rect(bx, btn_y, btn_w, btn_h, VxTheme::RADIUS_SM, bg);
        draw_abstract_char(bx + 12, btn_y + 12, symbol[0], active ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_MUTED);
    };

    draw_btn(start_x + 10, "<", tab.history_idx > 0);
    if (clicked && mouse_x >= start_x + 10 && mouse_x < start_x + 42 && mouse_y >= btn_y && mouse_y < btn_y + btn_h && tab.history_idx > 0) {
        tab.history_idx--; restore_history_idx();
    }
    
    draw_btn(start_x + 46, ">", tab.history_idx < tab.history_count - 1);
    if (clicked && mouse_x >= start_x + 46 && mouse_x < start_x + 78 && mouse_y >= btn_y && mouse_y < btn_y + btn_h && tab.history_idx < tab.history_count - 1) {
        tab.history_idx++; restore_history_idx();
    }
    
    draw_btn(start_x + 82, "R", true); // Reload

    // Address bar
    int addr_x = start_x + 124;
    int addr_w = width - 124 - 10;
    if (addr_w > 0) {
        bool addr_hover = (mouse_x >= addr_x && mouse_x < addr_x + addr_w && mouse_y >= btn_y && mouse_y < btn_y + btn_h);
        int text_x = addr_x + 28;
        
        if (clicked) {
            url_focused = addr_hover;
            if (addr_hover) {
                if (last_click_frame != 1000000 && frame >= last_click_frame && frame - last_click_frame < 25) {
                    tab.url_input.select_all();
                    last_click_frame = 1000000;
                } else {
                    int clicked_char = (mouse_x - text_x + 4) / 8;
                    if (clicked_char < 0) clicked_char = 0;
                    if (clicked_char > tab.url_len) clicked_char = tab.url_len;
                    tab.url_input.caret_pos = clicked_char;
                    tab.url_input.selection_anchor = tab.url_input.caret_pos;
                    last_click_frame = frame;
                }
            } else {
                tab.url_input.selection_anchor = tab.url_input.caret_pos;
            }
        }
        
        uint32_t addr_border = (url_focused && w.focused) ? VxTheme::accent() : VxTheme::BORDER_SUBTLE;
        vxui_draw_rounded_rect(addr_x, btn_y, addr_w, btn_h, VxTheme::RADIUS_MD, addr_hover ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH);
        vxr_fill_rect(addr_x, btn_y, addr_w, 1, addr_border);
        vxr_fill_rect(addr_x, btn_y + btn_h - 1, addr_w, 1, addr_border);
        vxr_fill_rect(addr_x, btn_y, 1, btn_h, addr_border);
        vxr_fill_rect(addr_x + addr_w - 1, btn_y, 1, btn_h, addr_border);
        
        // Lock Icon
        draw_abstract_char(addr_x + 12, btn_y + 12, 'L', VxTheme::SUCCESS);
        
        int text_y = btn_y + 12;
        
        VxClipRect uc = g_vxr_ctx.push_clip(addr_x + 26, btn_y + 2, addr_w - 30, btn_h - 4);
        
        if (tab.url_input.sel_active()) {
            int s = tab.url_input.sel_min(), e = tab.url_input.sel_max();
            if (s < 0) s = 0; if (e > tab.url_len) e = tab.url_len;
            if (e > s) vxr_fill_rect(text_x + s * 8, btn_y + 4, (e - s) * 8, btn_h - 8, VxTheme::accent_soft());
        }
        
        for (int i = 0; i < tab.url_len; i++) {
            draw_abstract_char(text_x + i * 8, text_y, tab.url_buffer[i], VxTheme::TEXT_PRIMARY);
        }
        
        if (w.focused && url_focused && (frame % 60 < 30) && !tab.url_input.sel_active()) {
            vxr_fill_rect(text_x + tab.url_input.caret_pos * 8, text_y, 2, 12, VxTheme::accent());
        }
        
        g_vxr_ctx.pop_clip(uc);
    }

    // Content area
    int content_y = toolbar_y + toolbar_h;
    int content_h = height - tab_h - toolbar_h;
    if (content_h <= 0) return;

    if (tab.current_page == 0) {
        // Home page
        vxr_fill_rect(start_x, content_y, width, content_h, VxTheme::BASE_DEEP);
        
        int hero_w = 400;
        int hero_h = 200;
        int hero_x = start_x + (width - hero_w) / 2;
        int hero_y = content_y + (content_h - hero_h) / 3;
        
        vxui_draw_shadow(hero_x, hero_y, hero_w, hero_h, 8);
        vxui_draw_rounded_rect(hero_x, hero_y, hero_w, hero_h, VxTheme::RADIUS_LG, VxTheme::SURFACE);
        
        const char* h1 = "Vextryn Air Web";
        for (int i=0; h1[i]; i++) draw_abstract_char(hero_x + (hero_w - 15*8)/2 + i*8, hero_y + 40, h1[i], VxTheme::TEXT_PRIMARY);
        
        int s_x = hero_x + 40;
        int s_y = hero_y + 100;
        int s_w = hero_w - 80;
        int s_h = 40;
        
        bool s_hover = (mouse_x >= s_x && mouse_x < s_x + s_w && mouse_y >= s_y && mouse_y < s_y + s_h);
        if (clicked) {
            search_focused = s_hover;
            if (search_focused) {
                int clicked_char = (mouse_x - (s_x + 12) + 4) / 8;
                if (clicked_char < 0) clicked_char = 0;
                if (clicked_char > tab.search_len) clicked_char = tab.search_len;
                tab.search_input.caret_pos = clicked_char;
                tab.search_input.selection_anchor = clicked_char;
            }
        }
        
        uint32_t s_border = (search_focused && w.focused) ? VxTheme::accent() : VxTheme::BORDER_BRIGHT;
        vxr_fill_rect(s_x, s_y, s_w, s_h, VxTheme::SURFACE);
        vxr_fill_rect(s_x, s_y, s_w, 1, s_border);
        vxr_fill_rect(s_x, s_y + s_h - 1, s_w, 1, s_border);
        vxr_fill_rect(s_x, s_y, 1, s_h, s_border);
        vxr_fill_rect(s_x + s_w - 1, s_y, 1, s_h, s_border);
        
        VxClipRect sc = g_vxr_ctx.push_clip(s_x + 2, s_y + 2, s_w - 4, s_h - 4);
        
        if (tab.search_len == 0 && !search_focused) {
            const char* ph = "Search...";
            for (int i=0; ph[i]; i++) draw_abstract_char(s_x + 12 + i*8, s_y + 14, ph[i], VxTheme::TEXT_MUTED);
        } else {
            if (tab.search_input.sel_active()) {
                int s = tab.search_input.sel_min(), e = tab.search_input.sel_max();
                if (s < 0) s = 0; if (e > tab.search_len) e = tab.search_len;
                if (e > s) vxr_fill_rect(s_x + 12 + s*8, s_y + 8, (e-s)*8, s_h - 16, VxTheme::accent_soft());
            }
            for (int i=0; i<tab.search_len; i++) {
                draw_abstract_char(s_x + 12 + i*8, s_y + 14, tab.search_buffer[i], VxTheme::TEXT_PRIMARY);
            }
            if (w.focused && search_focused && (frame % 60 < 30) && !tab.search_input.sel_active()) {
                vxr_fill_rect(s_x + 12 + tab.search_input.caret_pos * 8, s_y + 14, 2, 12, VxTheme::accent());
            }
        }
        g_vxr_ctx.pop_clip(sc);
        
        VxButton btn = { hero_x + (hero_w - 120)/2, hero_y + 160, 120, 36, "I'm Feeling Lucky", VX_BTN_PRIMARY, false, false, false, false };
        btn.check_hover(mouse_x, mouse_y);
        btn.draw();
        
    } else {
        // Mock rendering for a page
        vxr_fill_rect(start_x, content_y, width, content_h, 0xFFFFFFFF); // White background for web
        
        const char* line1 = "Welcome to the Mock Page!";
        const char* line2 = "This page is rendered natively by VxWeb engine.";
        const char* line3 = "Features: HTML parsing, CSS (coming soon).";
        
        for (int i=0; line1[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 40, line1[i], 0xFF000000);
        for (int i=0; line2[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 80, line2[i], 0xFF333333);
        for (int i=0; line3[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 120, line3[i], 0xFF333333);
        
        vxr_fill_rect(start_x + 40, content_y + 160, 200, 150, 0xFFE0E0E0);
        const char* img_mock = "[Image Placeholder]";
        for (int i=0; img_mock[i]; i++) draw_abstract_char(start_x + 60 + i*8, content_y + 220, img_mock[i], 0xFF666666);
    }
}
