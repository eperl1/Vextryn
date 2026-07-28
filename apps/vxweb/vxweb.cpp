#include "../../gui/vxui/vxui.hpp"
#include "../../net/core/vxair_socket.h"
#include "../../userspace/libc/include/vxlibc.h"

namespace Vx::Web {
struct HttpResponse {
    int status_code;
    char content_type[128];
    char* body;
    size_t body_len;
};

struct DOMNode {
    char tag[32];
    char text[1024];
    bool is_text;
    DOMNode* children[64];
    int child_count;
};

class VxWebBrowser {
private:
    char url[1024];
    char history[64][1024];
    int history_pos;
    int history_count;
    char page_content[65536];

    HttpResponse fetch(const char* url) {
        HttpResponse resp = {0};
        char host[256] = {0};
        char path[512] = "/";
        const char* p = url;
        
        // Mock rendering if it starts with "mock://"
        if (vxlibc_strncmp(p, "mock://", 7) == 0) {
            resp.status_code = 200;
            static char mock_body[] = "<html><body><h1>Welcome to Vextryn Air Web</h1>"
                "<p>This is a <b>serious</b> browser mockup.</p>"
                "<ul><li>Fast</li><li>Secure</li><li>Premium</li></ul>"
                "<table><tr><td>Row 1</td><td>Data</td></tr></table>"
                "</body></html>";
            resp.body = mock_body;
            resp.body_len = vxlibc_strlen(mock_body);
            return resp;
        }

        if (vxlibc_strncmp(p,"http://",7)==0) p += 7;
        int i = 0;
        while (*p && *p != '/' && i < 255) host[i++] = *p++;
        if (*p == '/') {
            int j = 0;
            while (*p && j < 511) path[j++] = *p++;
            path[j] = 0;
        }
        uint32_t ip = 0;
        vxair_dns_resolve(host, &ip);
        if (!ip) { resp.status_code = -1; return resp; }
        int fd = vxair_socket(2, 1, 0);
        struct vxair_sockaddr_in addr;
        addr.sin_family = 2; addr.sin_port = 0x5000; addr.sin_addr = ip;
        if (vxair_connect(fd, (struct vxair_sockaddr*)&addr, sizeof(addr)) < 0) {
            resp.status_code = -2; return resp;
        }
        char req[1024];
        vxlibc_snprintf(req, sizeof(req),
            "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
        vxair_send(fd, req, vxlibc_strlen(req), 0);
        static char buf[65536];
        int total = 0, n;
        while ((n = vxair_recv(fd, buf + total, sizeof(buf)-total-1, 0)) > 0) total += n;
        buf[total] = 0; vxair_close(fd);
        resp.status_code = 200;
        char* body = vxlibc_strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4; resp.body = body; resp.body_len = total - (body - buf);
        }
        return resp;
    }

    void render_html(const char* html, char* out, size_t out_size) {
        // A much more advanced parser mockup
        int i = 0, o = 0; 
        bool in_tag = false;
        char current_tag[32] = {0};
        int tag_idx = 0;
        bool is_closing = false;
        
        while (html[i] && o < (int)out_size-1) {
            if (html[i] == '<') {
                in_tag = true;
                tag_idx = 0;
                is_closing = (html[i+1] == '/');
                if (is_closing) i++; // skip '/'
            } else if (html[i] == '>') {
                in_tag = false;
                current_tag[tag_idx] = 0;
                
                if (vxlibc_strcasecmp(current_tag, "br") == 0 || 
                    vxlibc_strcasecmp(current_tag, "p") == 0 ||
                    vxlibc_strcasecmp(current_tag, "h1") == 0 ||
                    vxlibc_strcasecmp(current_tag, "h2") == 0 ||
                    vxlibc_strcasecmp(current_tag, "div") == 0 ||
                    vxlibc_strcasecmp(current_tag, "li") == 0 ||
                    vxlibc_strcasecmp(current_tag, "tr") == 0) {
                    if (o > 0 && out[o-1] != '\n') {
                        out[o++] = '\n';
                    }
                }
                
                if (vxlibc_strcasecmp(current_tag, "h1") == 0 && !is_closing) {
                    out[o++] = '#'; out[o++] = ' ';
                } else if (vxlibc_strcasecmp(current_tag, "li") == 0 && !is_closing) {
                    out[o++] = ' '; out[o++] = '*'; out[o++] = ' ';
                } else if (vxlibc_strcasecmp(current_tag, "td") == 0 && !is_closing) {
                    out[o++] = '|'; out[o++] = ' ';
                } else if (vxlibc_strcasecmp(current_tag, "td") == 0 && is_closing) {
                    out[o++] = ' '; out[o++] = '|'; out[o++] = '\t';
                }
            } else if (in_tag) {
                if (tag_idx < 31 && html[i] != ' ') {
                    current_tag[tag_idx++] = html[i];
                }
            } else {
                // Ignore newlines in HTML source to format cleanly
                if (html[i] != '\n' && html[i] != '\r') {
                    out[o++] = html[i];
                }
            }
            i++;
        }
        out[o] = 0;
    }

public:
    VxWebBrowser() : history_pos(0), history_count(0) {
        vxlibc_memset(url, 0, sizeof(url));
        vxlibc_memset(page_content, 0, sizeof(page_content));
        const char* home = "=== VEXTRYN AIR BROWSER ===\n\nWelcome to vxweb!\nType a URL and press Enter.\n\nTry mock://test for local render.\n";
        vxlibc_strncpy(page_content, home, sizeof(page_content));
    }
    void navigate(const char* new_url) {
        vxlibc_strncpy(url, new_url, sizeof(url));
        vxlibc_strncpy(history[history_count % 64], new_url, 1024);
        history_count++; history_pos = history_count;
        
        HttpResponse resp = fetch(new_url);
        
        if (resp.status_code == 200 && resp.body) {
            render_html(resp.body, page_content, sizeof(page_content));
        } else {
            vxlibc_snprintf(page_content, sizeof(page_content), "Error: Could not load %s\nStatus: %d\n", new_url, resp.status_code);
        }
    }
    void back() {
        if (history_pos > 1) { history_pos--; navigate(history[(history_pos-1) % 64]); }
    }
    const char* get_content() { return page_content; }
    const char* get_url() { return url; }
};
}
extern "C" int vxweb_main(int argc, char** argv) {
    Vx::Web::VxWebBrowser browser;
    if (argc > 1) browser.navigate(argv[1]);
    vxlibc_printf("==========================\n");
    vxlibc_printf(" URL: %s\n", browser.get_url());
    vxlibc_printf("==========================\n");
    vxlibc_printf("%s\n", browser.get_content());
    vxlibc_printf("==========================\n");
    return 0;
}
