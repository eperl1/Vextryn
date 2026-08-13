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
static bool browser_dom_proof_done = false;

extern void vxair_log_info(const char* fmt, ...);

static void browser_fetch_url(BrowserTab& tab, const char* url);
static inline char browser_lower_char(char c);
static const char* browser_html_entity(const char* start, const char* end, char& out);
static bool browser_tag_attr_value(const char* tag, const char* attr, char* out, int out_cap);
static void browser_dom_dump_sample_once();

static bool browser_starts_with(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    int i = 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return false;
        i++;
    }
    return true;
}

static bool browser_parse_ipv4_literal(const char* text, uint32_t* out_ip) {
    if (!text || !out_ip) return false;
    uint32_t parts[4] = {0, 0, 0, 0};
    int part_idx = 0;
    int value = 0;
    bool has_digit = false;
    for (int i = 0;; i++) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            has_digit = true;
            value = value * 10 + (c - '0');
            if (value > 255) return false;
            continue;
        }
        if (c == '.' || c == 0) {
            if (!has_digit || part_idx >= 4) return false;
            parts[part_idx++] = (uint32_t)value;
            value = 0;
            has_digit = false;
            if (c == 0) break;
            continue;
        }
        return false;
    }
    if (part_idx != 4) return false;
    *out_ip = (parts[0]) | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
    return true;
}

static void browser_set_html(BrowserTab& tab, const char* html) {
    int i = 0;
    if (!html) {
        tab.html_content[0] = 0;
        return;
    }
    while (html[i] && i < 4095) {
        tab.html_content[i] = html[i];
        i++;
    }
    tab.html_content[i] = 0;
}

static void browser_append_html(char* dst, int& len, int cap, const char* text) {
    if (!dst || !text || len >= cap - 1) return;
    for (int i = 0; text[i] && len < cap - 1; i++) dst[len++] = text[i];
    dst[len] = 0;
}

static void browser_url_encode_component(const char* src, char* dst, int cap) {
    static const char* hex = "0123456789ABCDEF";
    int di = 0;
    if (!src || !dst || cap <= 0) return;
    for (int i = 0; src[i] && di < cap - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                    c == '-' || c == '_' || c == '.' || c == '~';
        if (safe) {
            dst[di++] = (char)c;
        } else if (di < cap - 3) {
            dst[di++] = '%';
            dst[di++] = hex[(c >> 4) & 0xF];
            dst[di++] = hex[c & 0xF];
        } else {
            break;
        }
    }
    dst[di] = 0;
}

static bool browser_fetch_http_into(BrowserTab& tab, uint32_t ip, uint16_t port, const char* host_header, const char* path) {
    int sock = vxair_socket(VXAIR_AF_INET, VXAIR_SOCK_STREAM, VXAIR_IPPROTO_TCP);
    if (sock < 0) return false;

    struct vxair_sockaddr_in addr;
    addr.sin_family = VXAIR_AF_INET;
    addr.sin_port = port;
    addr.sin_addr = ip;

    if (vxair_connect(sock, &addr, sizeof(addr)) != 0) {
        vxair_close(sock);
        return false;
    }

    static char req[1024];
    int ri = 0;
    const char* p1 = "GET ";
    for (int i = 0; p1[i] && ri < (int)sizeof(req) - 1; i++) req[ri++] = p1[i];
    for (int i = 0; path && path[i] && ri < (int)sizeof(req) - 1; i++) req[ri++] = path[i];
    const char* p2 = " HTTP/1.0\r\nHost: ";
    for (int i = 0; p2[i] && ri < (int)sizeof(req) - 1; i++) req[ri++] = p2[i];
    for (int i = 0; host_header && host_header[i] && ri < (int)sizeof(req) - 1; i++) req[ri++] = host_header[i];
    const char* p3 = "\r\nConnection: close\r\nUser-Agent: VextrynAir/0.2\r\n\r\n";
    for (int i = 0; p3[i] && ri < (int)sizeof(req) - 1; i++) req[ri++] = p3[i];
    req[ri] = 0;

    vxair_send(sock, req, ri, 0);

    static char resp[4096];
    int len = 0;
    while (len < 4095) {
        int got = vxair_recv(sock, resp + len, 4095 - len, 0);
        if (got <= 0) break;
        len += got;
    }
    vxair_close(sock);
    if (len <= 0) return false;

    resp[len] = 0;
    char* body = resp;
    for (int i = 0; i < len - 3; i++) {
        if (resp[i] == '\r' && resp[i + 1] == '\n' && resp[i + 2] == '\r' && resp[i + 3] == '\n') {
            body = resp + i + 4;
            break;
        }
    }
    browser_set_html(tab, body);
    return true;
}

static bool browser_fetch_via_chromium_bridge(BrowserTab& tab, const char* url) {
    uint32_t bridge_ip = 0;
    if (!browser_parse_ipv4_literal("10.0.2.2", &bridge_ip)) return false;

    static char encoded[384];
    browser_url_encode_component(url, encoded, sizeof(encoded));

    static char path[512];
    int pi = 0;
    const char* prefix = "/render?url=";
    for (int i = 0; prefix[i] && pi < (int)sizeof(path) - 1; i++) path[pi++] = prefix[i];
    for (int i = 0; encoded[i] && pi < (int)sizeof(path) - 1; i++) path[pi++] = encoded[i];
    path[pi] = 0;

    return browser_fetch_http_into(tab, bridge_ip, 8081, "10.0.2.2:8081", path);
}

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
    browser_dom_dump_sample_once();
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

enum BrowserDomNodeKind {
    BROWSER_DOM_ELEMENT = 1,
    BROWSER_DOM_TEXT = 2
};

struct BrowserDomNode {
    int kind;
    char tag[16];
    char text[256];
    char href[128];
    char src[128];
    char alt[128];
    int parent;
    int first_child;
    int last_child;
    int next_sibling;
};

struct BrowserDomTree {
    BrowserDomNode nodes[512];
    int count;
    int root;
    int body;
};

static void browser_dom_node_reset(BrowserDomNode& n) {
    n.kind = 0;
    n.tag[0] = 0;
    n.text[0] = 0;
    n.href[0] = 0;
    n.src[0] = 0;
    n.alt[0] = 0;
    n.parent = -1;
    n.first_child = -1;
    n.last_child = -1;
    n.next_sibling = -1;
}

static void browser_dom_tree_reset(BrowserDomTree& tree) {
    tree.count = 0;
    tree.root = -1;
    tree.body = -1;
    for (int i = 0; i < 512; i++) browser_dom_node_reset(tree.nodes[i]);
}

static int browser_dom_add_node(BrowserDomTree& tree, int kind, int parent) {
    if (tree.count >= 512) return -1;
    int idx = tree.count++;
    browser_dom_node_reset(tree.nodes[idx]);
    tree.nodes[idx].kind = kind;
    tree.nodes[idx].parent = parent;
    if (parent >= 0) {
        BrowserDomNode& p = tree.nodes[parent];
        if (p.first_child < 0) p.first_child = idx;
        else tree.nodes[p.last_child].next_sibling = idx;
        p.last_child = idx;
    }
    return idx;
}

static void browser_dom_copy_lower_tag(char* dst, int dst_cap, const char* src, int src_len) {
    int di = 0;
    for (int i = 0; i < src_len && di < dst_cap - 1; i++) {
        char c = browser_lower_char(src[i]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '/') break;
        dst[di++] = c;
    }
    dst[di] = 0;
}

static void browser_dom_copy_attr_value(const char* raw_tag, const char* attr, char* dst, int dst_cap) {
    dst[0] = 0;
    if (!raw_tag) return;
    browser_tag_attr_value(raw_tag, attr, dst, dst_cap);
}

static void browser_dom_append_text(BrowserDomTree& tree, int parent, const char* start, const char* end) {
    if (!start || !end || start >= end) return;
    char buf[256];
    int bi = 0;
    bool pending_space = false;
    bool seen_non_ws = false;
    const char* p = start;
    while (p < end && bi < 255) {
        char c = 0;
        const char* next = nullptr;
        if (*p == '&') next = browser_html_entity(p, end, c);
        if (!next) { c = *p; next = p + 1; }
        if (c == '\r' || c == '\n' || c == '\t') c = ' ';
        if (c == ' ') {
            if (seen_non_ws) pending_space = true;
        } else {
            if (pending_space && bi < 255) {
                buf[bi++] = ' ';
                pending_space = false;
            }
            buf[bi++] = c;
            seen_non_ws = true;
        }
        p = next;
    }
    while (bi > 0 && buf[bi - 1] == ' ') bi--;
    if (bi <= 0) return;
    int idx = browser_dom_add_node(tree, BROWSER_DOM_TEXT, parent);
    if (idx < 0) return;
    BrowserDomNode& n = tree.nodes[idx];
    for (int i = 0; i < bi; i++) n.text[i] = buf[i];
    n.text[bi] = 0;
}

static bool browser_dom_is_void_tag(const char* tag) {
    return browser_tag_eq(tag, "br") || browser_tag_eq(tag, "img");
}

static bool browser_dom_is_block_tag(const char* tag) {
    return browser_tag_eq(tag, "html") || browser_tag_eq(tag, "head") || browser_tag_eq(tag, "body") ||
           browser_tag_eq(tag, "div") || browser_tag_eq(tag, "p") || browser_tag_eq(tag, "h1") ||
           browser_tag_eq(tag, "h2") || browser_tag_eq(tag, "h3") || browser_tag_eq(tag, "h4") ||
           browser_tag_eq(tag, "h5") || browser_tag_eq(tag, "h6") || browser_tag_eq(tag, "ul") ||
           browser_tag_eq(tag, "li");
}

static bool browser_dom_is_inline_tag(const char* tag) {
    return browser_tag_eq(tag, "span") || browser_tag_eq(tag, "a");
}

static int browser_dom_find_body(BrowserDomTree& tree) {
    for (int i = 0; i < tree.count; i++) {
        if (tree.nodes[i].kind == BROWSER_DOM_ELEMENT && browser_tag_eq(tree.nodes[i].tag, "body")) return i;
    }
    return tree.root;
}

static void browser_dom_parse(BrowserDomTree& tree, const char* html) {
    browser_dom_tree_reset(tree);
    tree.root = browser_dom_add_node(tree, BROWSER_DOM_ELEMENT, -1);
    if (tree.root >= 0) {
        BrowserDomNode& root = tree.nodes[tree.root];
        root.tag[0] = 'h'; root.tag[1] = 't'; root.tag[2] = 'm'; root.tag[3] = 'l'; root.tag[4] = 0;
    }

    int stack[64];
    int top = 0;
    stack[top++] = tree.root;

    const char* p = html ? html : "";
    while (*p) {
        if (*p != '<') {
            const char* start = p;
            while (*p && *p != '<') p++;
            browser_dom_append_text(tree, stack[top - 1], start, p);
            continue;
        }

        if (p[1] == '!' && p[2] == '-' && p[3] == '-') {
            p += 4;
            while (*p && !(p[0] == '-' && p[1] == '-' && p[2] == '>')) p++;
            if (*p) p += 3;
            continue;
        }

        const char* tag_begin = ++p;
        bool closing = false;
        if (*p == '/') { closing = true; tag_begin = ++p; }
        while (*p && *p != '>') p++;
        const char* tag_end = p;
        if (*p == '>') p++;
        if (tag_begin >= tag_end) continue;

        while (tag_begin < tag_end && (*tag_begin == ' ' || *tag_begin == '\t' || *tag_begin == '\r' || *tag_begin == '\n')) tag_begin++;
        while (tag_end > tag_begin && (tag_end[-1] == ' ' || tag_end[-1] == '\t' || tag_end[-1] == '\r' || tag_end[-1] == '\n')) tag_end--;
        if (tag_begin >= tag_end) continue;

        char raw[256];
        int raw_len = 0;
        bool self_close = false;
        for (const char* q = tag_begin; q < tag_end && raw_len < 255; q++) raw[raw_len++] = *q;
        raw[raw_len] = 0;
        while (raw_len > 0 && (raw[raw_len - 1] == '/' || raw[raw_len - 1] == ' ' || raw[raw_len - 1] == '\t')) {
            if (raw[raw_len - 1] == '/') self_close = true;
            raw[--raw_len] = 0;
        }

        const char* name_start = raw;
        while (*name_start == ' ' || *name_start == '\t' || *name_start == '\r' || *name_start == '\n') name_start++;
        if (*name_start == 0) continue;

        char tag[32];
        int name_len = 0;
        while (name_start[name_len] && name_start[name_len] != ' ' && name_start[name_len] != '\t' && name_start[name_len] != '\r' && name_start[name_len] != '\n' && name_start[name_len] != '/') {
            name_len++;
        }
        if (name_len <= 0) continue;
        browser_dom_copy_lower_tag(tag, 32, name_start, name_len);

        const char* attrs = name_start + name_len;
        while (*attrs == ' ' || *attrs == '\t' || *attrs == '\r' || *attrs == '\n') attrs++;

        if (closing) {
            for (int i = top - 1; i > 0; i--) {
                int node_idx = stack[i];
                if (tree.nodes[node_idx].kind == BROWSER_DOM_ELEMENT && browser_tag_eq(tree.nodes[node_idx].tag, tag)) {
                    top = i;
                    break;
                }
            }
            continue;
        }

        int parent = stack[top - 1];
        int node = browser_dom_add_node(tree, BROWSER_DOM_ELEMENT, parent);
        if (node < 0) continue;
        BrowserDomNode& n = tree.nodes[node];
        int ti = 0;
        while (tag[ti] && ti < 15) { n.tag[ti] = tag[ti]; ti++; }
        n.tag[ti] = 0;
        browser_dom_copy_attr_value(raw, "href", n.href, 128);
        browser_dom_copy_attr_value(raw, "src", n.src, 128);
        browser_dom_copy_attr_value(raw, "alt", n.alt, 128);

        if (browser_tag_eq(tag, "body")) tree.body = node;

        if (!self_close && !browser_dom_is_void_tag(tag)) {
            if (top < 64) stack[top++] = node;
        }
    }
}

static void browser_dom_dump_indent(int depth) {
    (void)depth;
}

static void browser_dom_dump_node(BrowserDomTree& tree, int idx, int depth) {
    if (idx < 0 || idx >= tree.count) return;
    BrowserDomNode& n = tree.nodes[idx];
    char line[512];
    int pos = 0;
    int spaces = depth * 2;
    if (spaces > 60) spaces = 60;
    for (int i = 0; i < spaces && pos < 500; i++) line[pos++] = ' ';
    if (n.kind == BROWSER_DOM_TEXT) {
        const char* prefix = "TEXT \"";
        for (int i = 0; prefix[i] && pos < 500; i++) line[pos++] = prefix[i];
        for (int i = 0; n.text[i] && pos < 500; i++) line[pos++] = n.text[i];
        if (pos < 500) line[pos++] = '"';
        line[pos] = 0;
        vxair_log_info("%s", line);
        return;
    }
    line[pos++] = '<';
    for (int i = 0; n.tag[i] && pos < 500; i++) line[pos++] = n.tag[i];
    if (n.href[0] && pos < 500) {
        const char* mid = " href=\"";
        for (int i = 0; mid[i] && pos < 500; i++) line[pos++] = mid[i];
        for (int i = 0; n.href[i] && pos < 500; i++) line[pos++] = n.href[i];
        if (pos < 500) line[pos++] = '"';
    }
    if (n.src[0] && pos < 500) {
        const char* mid = " src=\"";
        for (int i = 0; mid[i] && pos < 500; i++) line[pos++] = mid[i];
        for (int i = 0; n.src[i] && pos < 500; i++) line[pos++] = n.src[i];
        if (pos < 500) line[pos++] = '"';
    }
    if (n.alt[0] && pos < 500) {
        const char* mid = " alt=\"";
        for (int i = 0; mid[i] && pos < 500; i++) line[pos++] = mid[i];
        for (int i = 0; n.alt[i] && pos < 500; i++) line[pos++] = n.alt[i];
        if (pos < 500) line[pos++] = '"';
    }
    if (pos < 500) line[pos++] = '>';
    line[pos] = 0;
    vxair_log_info("%s", line);
    int child = n.first_child;
    while (child >= 0) {
        int next = tree.nodes[child].next_sibling;
        browser_dom_dump_node(tree, child, depth + 1);
        child = next;
    }
}

static void browser_dom_dump_sample_once() {
    if (browser_dom_proof_done) return;
    browser_dom_proof_done = true;

    const char* sample =
        "<html><body><div class='outer'><div class='inner'><p>Alpha &amp; Beta "
        "<span>unclosed span</p><p>Literal &lt;script&gt; text"
        "<img src='photo.png' alt='Sunset'>"
        "<ul><li>One<li>Two</ul>"
        "</div></body></html>";

    static BrowserDomTree tree;
    browser_dom_parse(tree, sample);
    int root = tree.body >= 0 ? tree.body : tree.root;
    vxair_log_info("DOM PROOF: messy HTML sample parsed into DOM tree");
    browser_dom_dump_node(tree, root, 0);
    vxair_log_info("DOM PROOF: end");
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
    int bold_depth;
    int italic_depth;
    int link_depth;
    int code_depth;
    int pre_depth;
    int quote_depth;
    int skip_depth;
    int heading_level;
    int indent;
    int list_depth;
    int list_kind[8];
    int list_index[8];
    int list_top;
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

static int browser_render_effective_size(const BrowserRenderState& s) {
    if (s.pre_depth > 0 || s.code_depth > 0) return 12;
    switch (s.heading_level) {
        case 1: return 20;
        case 2: return 18;
        case 3: return 16;
        case 4: return 15;
        case 5: return 14;
        case 6: return 13;
        default: return s.base_size;
    }
}

static int browser_render_effective_line_h(const BrowserRenderState& s) {
    if (s.pre_depth > 0 || s.code_depth > 0) return 17;
    switch (s.heading_level) {
        case 1: return 28;
        case 2: return 25;
        case 3: return 22;
        case 4: return 21;
        case 5: return 20;
        case 6: return 19;
        default: return 18;
    }
}

static uint32_t browser_render_effective_color(const BrowserRenderState& s) {
    if (s.link_depth > 0) return s.accent;
    if (s.code_depth > 0) return 0xFFD5DDE8;
    if (s.quote_depth > 0) return s.muted;
    return s.fg;
}

static bool browser_render_effective_bold(const BrowserRenderState& s) {
    return s.bold_depth > 0 || s.heading_level > 0;
}

static void browser_render_apply_style(BrowserRenderState& s) {
    s.line_h = browser_render_effective_line_h(s);
    s.base_size = browser_render_effective_size(s);
    s.indent = s.quote_depth * 16 + s.list_depth * 18;
}

static const char* browser_html_entity(const char* start, const char* end, char& out) {
    if (!start || start >= end || *start != '&') return nullptr;
    const char* semi = start + 1;
    while (semi < end && *semi && *semi != ';' && (semi - start) <= 10) semi++;
    if (semi >= end || *semi != ';') return nullptr;
    int len = (int)(semi - start - 1);
    const char* ent = start + 1;
    if (len == 2 && ent[0] == 'l' && ent[1] == 't') { out = '<'; return semi + 1; }
    if (len == 2 && ent[0] == 'g' && ent[1] == 't') { out = '>'; return semi + 1; }
    if (len == 3 && ent[0] == 'a' && ent[1] == 'm' && ent[2] == 'p') { out = '&'; return semi + 1; }
    if (len == 4 && ent[0] == 'q' && ent[1] == 'u' && ent[2] == 'o' && ent[3] == 't') { out = '"'; return semi + 1; }
    if (len == 4 && ent[0] == 'a' && ent[1] == 'p' && ent[2] == 'o' && ent[3] == 's') { out = '\''; return semi + 1; }
    if (len == 5 && ent[0] == 'n' && ent[1] == 'b' && ent[2] == 's' && ent[3] == 'p') { out = ' '; return semi + 1; }
    return nullptr;
}

static bool browser_tag_attr_value(const char* tag, const char* attr, char* out, int out_cap) {
    out[0] = 0;
    if (!tag || !attr) return false;
    int attr_len = 0;
    while (attr[attr_len]) attr_len++;
    const char* p = tag;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '/') p++;
        if (!*p) break;
        const char* key = p;
        while (*p && *p != '=' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '>') p++;
        int key_len = (int)(p - key);
        if (key_len == attr_len) {
            bool match = true;
            for (int i = 0; i < attr_len; i++) {
                if (browser_lower_char(key[i]) != browser_lower_char(attr[i])) { match = false; break; }
            }
            if (match) {
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                if (*p != '=') return false;
                p++;
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                char quote = 0;
                if (*p == '"' || *p == '\'') quote = *p++;
                int oi = 0;
                while (*p && ((quote && *p != quote) || (!quote && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '>'))) {
                    if (oi < out_cap - 1) out[oi++] = *p;
                    p++;
                }
                out[oi] = 0;
                return true;
            }
        }
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '>') p++;
    }
    return false;
}

static void browser_render_word(BrowserRenderState& s, const char* word, int size, uint32_t color, bool bold, bool underline) {
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
    if (underline) {
        int underline_y = s.y + size + 1;
        vxr_fill_rect(s.x, underline_y, word_w, 1, color);
    }
    s.x += word_w + vx_text::text_width(size, " ");
}

static void browser_render_text_run(BrowserRenderState& s, const char* start, const char* end, bool preserve_whitespace) {
    if (!start || !end || start >= end) return;
    char word[192];
    int wi = 0;
    int size = browser_render_effective_size(s);
    uint32_t color = browser_render_effective_color(s);
    bool bold = browser_render_effective_bold(s);
    bool underline = s.link_depth > 0;
    const char* p = start;
    while (p < end && *p) {
        char c = 0;
        const char* next = nullptr;
        if (*p == '&') next = browser_html_entity(p, end, c);
        if (!next) { c = *p; next = p + 1; }
        if (c == '\r') c = '\n';

        if (preserve_whitespace) {
            if (c == '\n') {
                if (wi > 0) {
                    word[wi] = 0;
                    browser_render_word(s, word, size, color, bold, underline);
                    wi = 0;
                }
                browser_render_newline(s);
                p = next;
                continue;
            }
            if (c == ' ' || c == '\t') {
                if (wi > 0) {
                    word[wi] = 0;
                    browser_render_word(s, word, size, color, bold, underline);
                    wi = 0;
                }
                int sp = vx_text::text_width(size, " ");
                if (s.x + sp > s.right) browser_render_newline(s);
                else s.x += sp;
                p = next;
                continue;
            }
            if (wi < 191) word[wi++] = c;
            p = next;
            continue;
        }

        bool space = (c == ' ' || c == '\t' || c == '\r' || c == '\n');
        if (!space) {
            if (wi < 191) word[wi++] = c;
            p = next;
            continue;
        }
        if (wi > 0) {
            word[wi] = 0;
            browser_render_word(s, word, size, color, bold, underline);
            wi = 0;
        }
        if (c == '\n') {
            browser_render_newline(s);
        } else if (c == ' ' || c == '\t') {
            if (s.x > s.left + s.indent) {
                int sp = vx_text::text_width(size, " ");
                if (s.x + sp > s.right) browser_render_newline(s);
                else s.x += sp;
            }
        }
        p = next;
    }
    if (wi > 0) {
        word[wi] = 0;
        browser_render_word(s, word, size, color, bold, underline);
    }
}

static void browser_render_plain_segment(BrowserRenderState& s, const char* start, const char* end) {
    browser_render_text_run(s, start, end, s.pre_depth > 0);
}

static void browser_render_block_break(BrowserRenderState& s, int before_gap, int after_gap) {
    browser_render_gap(s, before_gap);
    browser_render_gap(s, after_gap);
}

static int browser_heading_level_from_tag(const char* tag) {
    if (browser_tag_eq(tag, "h1")) return 1;
    if (browser_tag_eq(tag, "h2")) return 2;
    if (browser_tag_eq(tag, "h3")) return 3;
    if (browser_tag_eq(tag, "h4")) return 4;
    if (browser_tag_eq(tag, "h5")) return 5;
    if (browser_tag_eq(tag, "h6")) return 6;
    return 0;
}

static bool browser_tag_is_block(const char* tag) {
    return browser_tag_eq(tag, "p") || browser_tag_eq(tag, "div") || browser_tag_eq(tag, "section") || browser_tag_eq(tag, "article") ||
           browser_tag_eq(tag, "main") || browser_tag_eq(tag, "header") || browser_tag_eq(tag, "footer") || browser_tag_eq(tag, "nav") ||
           browser_tag_eq(tag, "aside") || browser_tag_eq(tag, "address") || browser_tag_eq(tag, "table") || browser_tag_eq(tag, "tr");
}

static int browser_heading_level_from_name(const char* tag) {
    if (browser_tag_eq(tag, "h1")) return 1;
    if (browser_tag_eq(tag, "h2")) return 2;
    if (browser_tag_eq(tag, "h3")) return 3;
    if (browser_tag_eq(tag, "h4")) return 4;
    if (browser_tag_eq(tag, "h5")) return 5;
    if (browser_tag_eq(tag, "h6")) return 6;
    return 0;
}

static void browser_render_text_cstr(BrowserRenderState& s, const char* text) {
    if (!text || !text[0]) return;
    const char* end = text;
    while (*end) end++;
    browser_render_text_run(s, text, end, false);
}

static void browser_render_dom_children(BrowserDomTree& tree, int parent_idx, BrowserRenderState& s);

static void browser_render_dom_node(BrowserDomTree& tree, int idx, BrowserRenderState& s) {
    if (idx < 0 || idx >= tree.count) return;
    BrowserDomNode& n = tree.nodes[idx];
    if (n.kind == BROWSER_DOM_TEXT) {
        browser_render_text_cstr(s, n.text);
        return;
    }

    const char* tag = n.tag;
    if (browser_tag_eq(tag, "html") || browser_tag_eq(tag, "body")) {
        browser_render_dom_children(tree, idx, s);
        return;
    }
    if (browser_tag_eq(tag, "head")) {
        return;
    }
    if (browser_tag_eq(tag, "br")) {
        browser_render_newline(s);
        return;
    }
    if (browser_tag_eq(tag, "img")) {
        if (n.alt[0]) {
            browser_render_text_cstr(s, n.alt);
        } else if (n.src[0]) {
            browser_render_text_cstr(s, "[image]");
        } else {
            browser_render_text_cstr(s, "[image]");
        }
        return;
    }

    if (browser_heading_level_from_name(tag) > 0) {
        int old_heading = s.heading_level;
        int lvl = browser_heading_level_from_name(tag);
        browser_render_block_break(s, lvl == 1 ? 10 : lvl == 2 ? 8 : 6, 0);
        s.heading_level = lvl;
        browser_render_apply_style(s);
        browser_render_dom_children(tree, idx, s);
        s.heading_level = old_heading;
        browser_render_apply_style(s);
        browser_render_block_break(s, 6, 10);
        return;
    }

    if (browser_tag_eq(tag, "p") || browser_tag_eq(tag, "div")) {
        browser_render_block_break(s, s.block_gap, 0);
        browser_render_dom_children(tree, idx, s);
        browser_render_block_break(s, 0, s.block_gap);
        return;
    }

    if (browser_tag_eq(tag, "span")) {
        browser_render_dom_children(tree, idx, s);
        return;
    }

    if (browser_tag_eq(tag, "a")) {
        s.link_depth++;
        browser_render_apply_style(s);
        browser_render_dom_children(tree, idx, s);
        if (s.link_depth > 0) s.link_depth--;
        browser_render_apply_style(s);
        return;
    }

    if (browser_tag_eq(tag, "ul")) {
        browser_render_block_break(s, 6, 2);
        if (s.list_depth < 8) s.list_depth++;
        browser_render_apply_style(s);
        browser_render_dom_children(tree, idx, s);
        if (s.list_depth > 0) s.list_depth--;
        browser_render_apply_style(s);
        browser_render_block_break(s, 2, 6);
        return;
    }

    if (browser_tag_eq(tag, "li")) {
        int depth = s.list_depth > 0 ? s.list_depth - 1 : 0;
        int saved_indent = s.indent;
        browser_render_newline(s, 2);
        s.indent = 16 + depth * 18;
        s.x = s.left + s.indent;
        vxr_circle(s.left + 8 + depth * 18, s.y + 8, 2, s.accent);
        browser_render_dom_children(tree, idx, s);
        browser_render_newline(s, 2);
        s.indent = saved_indent;
        s.x = s.left + s.indent;
        return;
    }

    browser_render_dom_children(tree, idx, s);
}

static void browser_render_dom_children(BrowserDomTree& tree, int parent_idx, BrowserRenderState& s) {
    if (parent_idx < 0 || parent_idx >= tree.count) return;
    int child = tree.nodes[parent_idx].first_child;
    while (child >= 0) {
        int next = tree.nodes[child].next_sibling;
        browser_render_dom_node(tree, child, s);
        child = next;
    }
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
    s.bold_depth = 0;
    s.italic_depth = 0;
    s.link_depth = 0;
    s.code_depth = 0;
    s.pre_depth = 0;
    s.quote_depth = 0;
    s.skip_depth = 0;
    s.heading_level = 0;
    s.indent = 0;
    s.list_depth = 0;
    s.list_top = 0;
    s.block_gap = 10;
    for (int i = 0; i < 8; i++) {
        s.list_kind[i] = 0;
        s.list_index[i] = 0;
    }
    browser_render_apply_style(s);

    vxr_fill_rect(x, y, w, h, page_bg);
    vxr_circle(x + w - 90, y + 44, 180, 0x180A84FF);
    vxr_circle(x + 70, y + h - 20, 150, 0x1400D7FF);
    vxui_draw_rounded_rect(x + 18, y + 16, w - 36, h - 32, 12, 0xEC161B24);
    vxr_rounded_border(x + 18, y + 16, w - 36, h - 32, 12, 0x35D9DEE7);
    vxr_fill_rect(x + 36, y + 58, w - 72, 1, 0x28D9DEE7);

    VxClipRect clip = g_vxr_ctx.push_clip(x + 24, y + 24, w - 48, h - 48);
    static BrowserDomTree tree;
    browser_dom_parse(tree, tab.html_content);
    int render_root = tree.body >= 0 ? tree.body : tree.root;
    if (render_root >= 0) browser_render_dom_children(tree, render_root, s);
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
    const char* path = "/";
    if (!host || !host[0]) {
        browser_set_html(tab, "<html><body><h1>Empty URL</h1><p>Enter an address to browse.</p></body></html>");
        return;
    }

    if (browser_starts_with(host, "http://")) {
        host += 7;
        const char* slash = host;
        while (*slash && *slash != '/') slash++;
        if (*slash == '/') path = slash;
    } else if (browser_starts_with(host, "https://")) {
        host += 8;
        const char* slash = host;
        while (*slash && *slash != '/') slash++;
        if (*slash == '/') path = slash;
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
        while (fallback[hi] && hi < 95) { hostname[hi] = fallback[hi]; hi++; }
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
                static char req[256];
                int ri = 0;
                const char* p1 = "GET ";
                for (int i = 0; p1[i] && ri < 255; i++) req[ri++] = p1[i];
                for (int i = 0; path[i] && ri < 255; i++) req[ri++] = path[i];
                const char* p2 = " HTTP/1.0\r\nHost: ";
                for (int i = 0; p2[i] && ri < 255; i++) req[ri++] = p2[i];
                for (int i = 0; hostname[i] && ri < 255; i++) req[ri++] = hostname[i];
                const char* p3 = "\r\nConnection: close\r\nUser-Agent: VextrynAir/0.2\r\n\r\n";
                for (int i = 0; p3[i] && ri < 255; i++) req[ri++] = p3[i];
                req[ri] = 0;
                vxair_send(sock, req, ri, 0);

                static char resp[4096];
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
                    browser_set_html(tab, body);
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
