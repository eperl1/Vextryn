import re

with open('gui/compositor/apps/app_browser.hpp', 'r') as f:
    code = f.read()

struct_repl = """    char html_content[2048];
};
"""
code = code.replace('};', struct_repl, 1)

include_repl = """#include "../vxair_textinput.hpp"
#include "../../net/core/socket.h"
"""
code = code.replace('#include "../vxair_textinput.hpp"', include_repl)

init_repl = """    tab.search_input.init(tab.search_buffer, &tab.search_len, 128);
    tab.html_content[0] = 0;"""
code = code.replace('tab.search_input.init(tab.search_buffer, &tab.search_len, 128);', init_repl)


fetch_func = """static void browser_fetch_url(BrowserTab& tab, const char* url) {
    uint32_t ip;
    if (vxair_dns_resolve("mock.com", &ip) == 0) {
        int sock = vxair_socket(VXAIR_AF_INET, VXAIR_SOCK_STREAM, VXAIR_IPPROTO_TCP);
        if (sock >= 0) {
            struct vxair_sockaddr_in addr;
            addr.sin_family = VXAIR_AF_INET;
            addr.sin_port = 80;
            addr.sin_addr = ip;
            if (vxair_connect(sock, &addr, sizeof(addr)) == 0) {
                const char* req = "GET / HTTP/1.0\\r\\n\\r\\n";
                vxair_send(sock, req, 18, 0);
                
                char resp[2048];
                int len = vxair_recv(sock, resp, 2047, 0);
                if (len > 0) {
                    resp[len] = 0;
                    char* body = resp;
                    for (int i = 0; i < len - 3; i++) {
                        if (resp[i] == '\\r' && resp[i+1] == '\\n' && resp[i+2] == '\\r' && resp[i+3] == '\\n') {
                            body = resp + i + 4;
                            break;
                        }
                    }
                    int b_idx = 0;
                    while(*body && b_idx < 2047) tab.html_content[b_idx++] = *body++;
                    tab.html_content[b_idx] = 0;
                }
            }
            vxair_close(sock);
        }
    }
}
"""
code = code.replace('static void browser_handle_key(char c) {', fetch_func + '\nstatic void browser_handle_key(char c) {')

# Add fetch call
fetch_call = """            tab.current_page = 1;
            browser_fetch_url(tab, tab.url_buffer);"""
code = code.replace('tab.current_page = 1;', fetch_call, 2)


# Replace mock render
mock_render_search = """        const char* line1 = "Welcome to the Mock Page!";
        const char* line2 = "This page is rendered natively by VxWeb engine.";
        const char* line3 = "Features: HTML parsing, CSS (coming soon).";
        
        for (int i=0; line1[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 40, line1[i], 0xFF000000);
        for (int i=0; line2[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 80, line2[i], 0xFF333333);
        for (int i=0; line3[i]; i++) draw_abstract_char(start_x + 40 + i*8, content_y + 120, line3[i], 0xFF333333);
        
        vxr_fill_rect(start_x + 40, content_y + 160, 200, 150, 0xFFE0E0E0);
        const char* img_mock = "[Image Placeholder]";
        for (int i=0; img_mock[i]; i++) draw_abstract_char(start_x + 60 + i*8, content_y + 220, img_mock[i], 0xFF666666);"""

mock_render_replace = """        int dx = start_x + 40;
        int dy = content_y + 40;
        int i = 0;
        if (tab.html_content[0] == 0) {
            const char* err = "No connection / Empty page";
            for (int k=0; err[k]; k++) draw_abstract_char(dx + k*8, dy, err[k], 0xFF000000);
        }
        while(tab.html_content[i]) {
            if (tab.html_content[i] == '<') {
                if (tab.html_content[i+1] == 'h' && tab.html_content[i+2] == '1' && tab.html_content[i+3] == '>') {
                    i += 4;
                    while(tab.html_content[i] && !(tab.html_content[i] == '<' && tab.html_content[i+1] == '/')) {
                        draw_abstract_char(dx, dy, tab.html_content[i], 0xFF000000);
                        dx += 8;
                        i++;
                    }
                    dy += 40;
                    dx = start_x + 40;
                } else if (tab.html_content[i+1] == 'p' && tab.html_content[i+2] == '>') {
                    i += 3;
                    while(tab.html_content[i] && !(tab.html_content[i] == '<' && tab.html_content[i+1] == '/')) {
                        draw_abstract_char(dx, dy, tab.html_content[i], 0xFF333333);
                        dx += 8;
                        if (dx > start_x + width - 40) { dx = start_x + 40; dy += 16; }
                        i++;
                    }
                    dy += 24;
                    dx = start_x + 40;
                }
                while(tab.html_content[i] && tab.html_content[i] != '>') i++;
                if (tab.html_content[i] == '>') i++;
            } else {
                i++;
            }
        }"""
code = code.replace(mock_render_search, mock_render_replace)

with open('gui/compositor/apps/app_browser.hpp', 'w') as f:
    f.write(code)
