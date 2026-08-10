#pragma once

#include "../../vxui/vxui_advanced.hpp"
#include "../../vxui/vx_text.hpp"
#include "../vxair_textinput.hpp"
#include "../../../net/core/socket.h"


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
    char html_content[4096];
};

#define MAX_BROWSER_TABS 8
static BrowserTab browser_tabs[MAX_BROWSER_TABS];
static int num_browser_tabs = 0;
static int active_browser_tab = 0;

static uint64_t last_click_frame = 1000000;
static bool url_focused = false;
static bool search_focused = false;

static void browser_fetch_url(BrowserTab& tab, const char* url);

static void create_browser_tab() {
    if (num_browser_tabs >= MAX_BROWSER_TABS) return;
    int idx = num_browser_tabs++;
    BrowserTab& tab = browser_tabs[idx];
    const char* default_url = "http://example.com";
    tab.url_len = 0;
    while (default_url[tab.url_len] && tab.url_len < 127) {
        tab.url_buffer[tab.url_len] = default_url[tab.url_len];
        tab.url_len++;
    }
    tab.url_buffer[tab.url_len] = 0;
    tab.url_input.init(tab.url_buffer, &tab.url_len, 128);
    tab.history_count = 1;
    tab.history_idx = 0;
    tab.history[0].page = 1;
    tab.history[0].url_len = tab.url_len;
    for (int i = 0; i < 128; i++) tab.history[0].url[i] = tab.url_buffer[i];
    tab.current_page = 1;
    tab.search_len = 0;
    tab.search_buffer[0] = 0;
    tab.search_input.init(tab.search_buffer, &tab.search_len, 128);
    tab.html_content[0] = 0;
    active_browser_tab = idx;
    url_focused = false;
    search_focused = false;
    browser_fetch_url(tab, tab.url_buffer);
}

static void restore_history_idx() {
    BrowserTab& tab = browser_tabs[active_browser_tab];
    tab.current_page = tab.history[tab.history_idx].page;
    tab.url_len = tab.history[tab.history_idx].url_len;
    for(int i=0; i<128; i++) tab.url_buffer[i] = tab.history[tab.history_idx].url[i];
}

static inline char browser_lower_char(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool browser_tag_eq(const char* tag, const char* name) {
    int i = 0;
    if (tag[0] == '/') i = 1;
    while (tag[i] == ' ' || tag[i] == '\t' || tag[i] == '\r' || tag[i] == '\n') i++;
    int j = 0;
    while (name[j]) {
        if (browser_lower_char(tag[i + j]) != browser_lower_char(name[j])) return false;
        j++;
    }
    char tail = tag[i + j];
    return tail == 0 || tail == ' ' || tail == '\t' || tail == '\r' || tail == '\n' || tail == '>' || tail == '/';
}

struct BrowserRenderState {
    int left;
    int right;
    int x;
    int y;
    int line_h;
    int base_size;
    uint32_t bg;
    uint32_t fg;
    uint32_t muted;
    uint32_t accent;
    bool bold;
    bool link;
    bool pre;
    int indent;
    int list_depth;
    int block_gap;
};

static void browser_render_newline(BrowserRenderState& s, int gap = 0) {
    s.x = s.left + s.indent;
    s.y += s.line_h + gap;
}

static void browser_render_gap(BrowserRenderState& s, int gap) {
    s.x = s.left + s.indent;
    s.y += gap;
}

static void browser_render_word(BrowserRenderState& s, const char* word, int size, uint32_t color, bool bold) {
    if (!word || !word[0]) return;
    int word_w = vx_text::text_width(size, word);
    if (s.x > s.left + s.indent && s.x + word_w > s.right) {
        browser_render_newline(s);
    }
    if (s.x + word_w > s.right && s.x <= s.left + s.indent) {
        word_w = vx_text::text_width(size, word);
    }
    if (bold) vx_text::draw_bold(s.x, s.y, size, word, color, s.bg);
    else vx_text::draw(s.x, s.y, size, word, color, s.bg);
    if (s.link) {
        int underline_y = s.y + size + 1;
        vxr_fill_rect(s.x, underline_y, word_w, 1, color);
    }
    s.x += word_w + vx_text::text_width(size, " ");
}

static void browser_render_text_run(BrowserRenderState& s, const char* text, int size, uint32_t color, bool bold) {
    if (!text || !text[0]) return;
    char word[128];
    int wi = 0;
    for (int i = 0; ; i++) {
        char c = text[i];
        bool end = (c == 0);
        bool space = (!end && (c == ' ' || c == '\t' || c == '\r' || c == '\n'));
        if (!end && !space) {
            if (wi < 127) word[wi++] = c;
            continue;
        }
        if (wi > 0) {
            word[wi] = 0;
            browser_render_word(s, word, size, color, bold);
            wi = 0;
        }
        if (end) break;
        if (c == '\n') {
            browser_render_newline(s);
        } else if (c == ' ' || c == '\t') {
            if (s.x > s.left + s.indent) {
                int sp = vx_text::text_width(size, " ");
                if (s.x + sp > s.right) browser_render_newline(s);
                else s.x += sp;
            }
        }
    }
}

static void browser_render_plain_segment(BrowserRenderState& s, const char* start, const char* end, int size, uint32_t color, bool bold) {
    if (start >= end) return;
    char buf[256];
    int len = 0;
    for (const char* p = start; p < end && len < 255; p++) buf[len++] = *p;
    buf[len] = 0;
    browser_render_text_run(s, buf, size, color, bold);
}

static void browser_render_html(BrowserTab& tab, int x, int y, int w, int h) {
    const uint32_t page_bg = 0xFF121722;
    BrowserRenderState s;
    s.left = x + 24;
    s.right = x + w - 24;
    s.x = s.left;
    s.y = y + 22;
    s.line_h = 18;
    s.base_size = 13;
    s.bg = page_bg;
    s.fg = 0xFFE5E9F0;
    s.muted = 0xFFB0B8C4;
    s.accent = 0xFF0FE7FF;
    s.bold = false;
    s.link = false;
    s.pre = false;
    s.indent = 0;
    s.list_depth = 0;
    s.block_gap = 10;

    vxr_fill_rect(x, y, w, h, page_bg);
    vxr_circle(x + w - 90, y + 44, 180, 0x180A84FF);
    vxr_circle(x + 70, y + h - 20, 150, 0x1400D7FF);
    vxui_draw_rounded_rect(x + 18, y + 16, w - 36, h - 32, 12, 0xEC161B24);
    vxr_rounded_border(x + 18, y + 16, w - 36, h - 32, 12, 0x35D9DEE7);
    vxr_fill_rect(x + 36, y + 58, w - 72, 1, 0x28D9DEE7);

    VxClipRect clip = g_vxr_ctx.push_clip(x + 24, y + 24, w - 48, h - 48);

    const char* p = tab.html_content;
    const char* text_start = p;
    while (*p) {
        if (*p == '<') {
            browser_render_plain_segment(s, text_start, p, s.base_size, s.link ? s.accent : (s.bold ? s.fg : s.fg), s.bold);
            const char* tag_start = ++p;
            bool closing = false;
            if (*p == '/') { closing = true; tag_start = ++p; }
            char tag[48];
            int ti = 0;
            while (*p && *p != '>' && ti < 47) {
                tag[ti++] = browser_lower_char(*p);
                p++;
            }
            tag[ti] = 0;
            if (*p == '>') p++;
            text_start = p;

            if (!closing) {
                if (browser_tag_eq(tag, "br")) {
                    browser_render_newline(s);
                } else if (browser_tag_eq(tag, "p") || browser_tag_eq(tag, "div") || browser_tag_eq(tag, "section") || browser_tag_eq(tag, "article")) {
                    browser_render_gap(s, s.block_gap);
                    s.indent = 0;
                    s.x = s.left;
                } else if (browser_tag_eq(tag, "h1")) {
                    browser_render_gap(s, 10);
                    s.bold = true;
                    s.base_size = 17;
                    s.line_h = 24;
                    s.indent = 0;
                } else if (browser_tag_eq(tag, "h2")) {
                    browser_render_gap(s, 8);
                    s.bold = true;
                    s.base_size = 15;
                    s.line_h = 22;
                    s.indent = 0;
                } else if (browser_tag_eq(tag, "h3")) {
                    browser_render_gap(s, 6);
                    s.bold = true;
                    s.base_size = 14;
                    s.line_h = 20;
                    s.indent = 0;
                } else if (browser_tag_eq(tag, "li")) {
                    browser_render_newline(s, 2);
                    s.indent = 18 + s.list_depth * 14;
                    s.x = s.left + s.indent;
                    vxr_circle(s.left + 8 + s.list_depth * 6, s.y + 8, 2, s.accent);
                } else if (browser_tag_eq(tag, "ul") || browser_tag_eq(tag, "ol")) {
                    s.list_depth++;
                    browser_render_gap(s, 2);
                } else if (browser_tag_eq(tag, "pre")) {
                    browser_render_gap(s, 8);
                    s.pre = true;
                    s.base_size = 12;
                    s.line_h = 17;
                    vxui_draw_rounded_rect(s.left - 4, s.y - 4, s.right - s.left + 8, 28, 8, 0xFF0B0F14);
                } else if (browser_tag_eq(tag, "code")) {
                    s.bold = false;
                    s.base_size = 12;
                    s.link = false;
                } else if (browser_tag_eq(tag, "a")) {
                    s.link = true;
                } else if (browser_tag_eq(tag, "strong") || browser_tag_eq(tag, "b")) {
                    s.bold = true;
                } else if (browser_tag_eq(tag, "hr")) {
                    browser_render_gap(s, 10);
                    vxr_fill_rect(s.left, s.y + 6, s.right - s.left, 1, 0x28D9DEE7);
                    browser_render_gap(s, 16);
                }
            } else {
                if (browser_tag_eq(tag, "h1") || browser_tag_eq(tag, "h2") || browser_tag_eq(tag, "h3")) {
                    s.bold = false;
                    s.base_size = 13;
                    s.line_h = 18;
                    browser_render_gap(s, 10);
                } else if (browser_tag_eq(tag, "p") || browser_tag_eq(tag, "div") || browser_tag_eq(tag, "section") || browser_tag_eq(tag, "article")) {
                    browser_render_gap(s, s.block_gap);
                    s.x = s.left;
                } else if (browser_tag_eq(tag, "li")) {
                    s.indent = 0;
                    browser_render_newline(s, 4);
                } else if (browser_tag_eq(tag, "ul") || browser_tag_eq(tag, "ol")) {
                    if (s.list_depth > 0) s.list_depth--;
                    s.indent = 0;
                    browser_render_gap(s, 2);
                } else if (browser_tag_eq(tag, "pre")) {
                    s.pre = false;
                    s.base_size = 13;
                    s.line_h = 18;
                    browser_render_gap(s, 10);
                } else if (browser_tag_eq(tag, "a")) {
                    s.link = false;
                } else if (browser_tag_eq(tag, "strong") || browser_tag_eq(tag, "b")) {
                    s.bold = false;
                } else if (browser_tag_eq(tag, "code")) {
                    s.base_size = 13;
                }
            }
        } else {
            p++;
        }
    }
    browser_render_plain_segment(s, text_start, p, s.base_size, s.link ? s.accent : s.fg, s.bold);

    g_vxr_ctx.pop_clip(clip);
}

static void browser_render_home(BrowserTab& tab, int x, int y, int w, int h, int mouse_x, int mouse_y, bool clicked, bool window_focused, bool& search_focused_ref) {
    vxr_fill_rect(x, y, w, h, VxTheme::BASE_DEEP);

    int hero_w = 400;
    int hero_h = 200;
    int hero_x = x + (w - hero_w) / 2;
    int hero_y = y + (h - hero_h) / 3;

    vxui_draw_shadow(hero_x, hero_y, hero_w, hero_h, 8);
    vxui_draw_rounded_rect(hero_x, hero_y, hero_w, hero_h, VxTheme::RADIUS_LG, VxTheme::SURFACE);

    const char* h1 = "Vextryn Air Web";
    vx_text::draw_centered_bold(hero_x + hero_w / 2, hero_y + 34, 16, h1, VxTheme::TEXT_PRIMARY, VxTheme::SURFACE);

    int s_x = hero_x + 40;
    int s_y = hero_y + 100;
    int s_w = hero_w - 80;
    int s_h = 40;

    bool s_hover = (mouse_x >= s_x && mouse_x < s_x + s_w && mouse_y >= s_y && mouse_y < s_y + s_h);
    if (clicked) {
        search_focused_ref = s_hover;
        if (search_focused_ref) {
            int clicked_char = (mouse_x - (s_x + 12) + 4) / 8;
            if (clicked_char < 0) clicked_char = 0;
            if (clicked_char > tab.search_len) clicked_char = tab.search_len;
            tab.search_input.caret_pos = clicked_char;
            tab.search_input.selection_anchor = clicked_char;
        }
    }

    uint32_t s_border = (search_focused_ref && window_focused) ? VxTheme::accent() : VxTheme::BORDER_BRIGHT;
    vxr_fill_rect(s_x, s_y, s_w, s_h, VxTheme::SURFACE);
    vxr_fill_rect(s_x, s_y, s_w, 1, s_border);
    vxr_fill_rect(s_x, s_y + s_h - 1, s_w, 1, s_border);
    vxr_fill_rect(s_x, s_y, 1, s_h, s_border);
    vxr_fill_rect(s_x + s_w - 1, s_y, 1, s_h, s_border);

    VxClipRect sc = g_vxr_ctx.push_clip(s_x + 2, s_y + 2, s_w - 4, s_h - 4);

    if (tab.search_len == 0 && !search_focused_ref) {
        vx_text::draw(s_x + 12, s_y + 14, 13, "Search...", VxTheme::TEXT_MUTED, VxTheme::SURFACE);
    } else {
        if (tab.search_input.sel_active()) {
            int s = tab.search_input.sel_min(), e = tab.search_input.sel_max();
            if (s < 0) s = 0; if (e > tab.search_len) e = tab.search_len;
            if (e > s) vxr_fill_rect(s_x + 12 + s*8, s_y + 8, (e-s)*8, s_h - 16, VxTheme::accent_soft());
        }
        for (int i=0; i<tab.search_len; i++) {
            draw_abstract_char(s_x + 12 + i*8, s_y + 14, tab.search_buffer[i], VxTheme::TEXT_PRIMARY);
        }
        if (window_focused && search_focused_ref && (tab.search_input.sel_active() == false)) {
            vxr_fill_rect(s_x + 12 + tab.search_input.caret_pos * 8, s_y + 14, 2, 12, VxTheme::accent());
        }
    }
    g_vxr_ctx.pop_clip(sc);

    VxButton btn = { hero_x + (hero_w - 120)/2, hero_y + 160, 120, 36, "I'm Feeling Lucky", VX_BTN_PRIMARY, false, false, false, false };
    btn.check_hover(mouse_x, mouse_y);
    btn.draw();
}

static void browser_fetch_url(BrowserTab& tab, const char* url) {
    uint32_t ip;
    const char* host = url;
    if (host[0] == 'h' && host[1] == 't' && host[2] == 't' && host[3] == 'p') {
        while (*host && *host != '/') host++;
        if (host[0] == '/' && host[1] == '/') host += 2;
    }
    char hostname[96];
    int hi = 0;
    while (host[hi] && host[hi] != '/' && host[hi] != ':' && hi < 95) {
        hostname[hi] = host[hi];
        hi++;
    }
    hostname[hi] = 0;
    if (hostname[0] == 0) {
        const char* fallback = "example.com";
        hi = 0;
        while (fallback[hi]) { hostname[hi] = fallback[hi]; hi++; }
        hostname[hi] = 0;
    }

    if (vxair_dns_resolve(hostname, &ip) == 0) {
        int sock = vxair_socket(VXAIR_AF_INET, VXAIR_SOCK_STREAM, VXAIR_IPPROTO_TCP);
        if (sock >= 0) {
            struct vxair_sockaddr_in addr;
            addr.sin_family = VXAIR_AF_INET;
            addr.sin_port = 80;
            addr.sin_addr = ip;
            if (vxair_connect(sock, &addr, sizeof(addr)) == 0) {
                char req[256];
                int ri = 0;
                const char* p1 = "GET / HTTP/1.0\r\nHost: ";
                for (int i = 0; p1[i] && ri < 255; i++) req[ri++] = p1[i];
                for (int i = 0; hostname[i] && ri < 255; i++) req[ri++] = hostname[i];
                const char* p2 = "\r\nConnection: close\r\n\r\n";
                for (int i = 0; p2[i] && ri < 255; i++) req[ri++] = p2[i];
                req[ri] = 0;
                vxair_send(sock, req, ri, 0);

                char resp[4096];
                int len = 0;
                while (len < 4095) {
                    int got = vxair_recv(sock, resp + len, 4095 - len, 0);
                    if (got <= 0) break;
                    len += got;
                }
                if (len > 0) {
                    resp[len] = 0;
                    char* body = resp;
                    for (int i = 0; i < len - 3; i++) {
                        if (resp[i] == '\r' && resp[i+1] == '\n' && resp[i+2] == '\r' && resp[i+3] == '\n') {
                            body = resp + i + 4;
                            break;
                        }
                    }
                    int b_idx = 0;
                    while(*body && b_idx < 4095) tab.html_content[b_idx++] = *body++;
                    tab.html_content[b_idx] = 0;
                }
            }
            vxair_close(sock);
        }
    }
}

static void browser_handle_key(char c) {
    BrowserTab& tab = browser_tabs[active_browser_tab];
    if (url_focused) {
        if (c == '\n' || c == '\r') {
                        tab.current_page = 1;
            browser_fetch_url(tab, tab.url_buffer);
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
            browser_fetch_url(tab, tab.url_buffer);
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
        browser_render_home(tab, start_x, content_y, width, content_h, mouse_x, mouse_y, clicked, w.focused, search_focused);
    } else {
        if (tab.html_content[0] == 0) {
            vxr_fill_rect(start_x, content_y, width, content_h, 0xFF0E1117);
            vx_text::draw(start_x + 40, content_y + 44, 14, "No connection / Empty page", VxTheme::TEXT_MUTED, 0xFF0E1117);
        } else {
            browser_render_html(tab, start_x, content_y, width, content_h);
        }
    }
}