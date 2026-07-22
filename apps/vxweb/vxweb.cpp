#include "../../gui/vxui/vxui_widgets.hpp"
#include "../../net/core/vxair_socket.h"
#include "../../userspace/libc/include/vxlibc.h"

namespace Vx::Web {
struct HttpResponse {
    int status_code;
    char content_type[128];
    char* body;
    size_t body_len;
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
        int i = 0, o = 0; bool in_tag = false;
        while (html[i] && o < (int)out_size-1) {
            if (html[i] == '<') {
                in_tag = true;
                if (vxlibc_strncasecmp(html+i,"<br",3)==0 || vxlibc_strncasecmp(html+i,"<p",2)==0) out[o++] = '\n';
            } else if (html[i] == '>') {
                in_tag = false;
            } else if (!in_tag) {
                out[o++] = html[i];
            }
            i++;
        }
        out[o] = 0;
    }
public:
    VxWebBrowser() : history_pos(0), history_count(0) {
        vxlibc_memset(url, 0, sizeof(url));
        vxlibc_memset(page_content, 0, sizeof(page_content));
        const char* home = "=== VEXTRYN AIR BROWSER ===\n\nWelcome to vxweb!\nType a URL and press Enter.\n\nExample: http://10.0.2.2/\n";
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
    vxlibc_printf("=== vxweb ===\n");
    vxlibc_printf("URL: %s\n", browser.get_url());
    vxlibc_printf("Content:\n%s\n", browser.get_content());
    return 0;
}
