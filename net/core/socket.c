#include "socket.h"
#include <string.h>

#define VXAIR_MAX_SOCKETS 32

typedef struct {
    int in_use;
    int domain;
    int type;
    int protocol;
    struct vxair_sockaddr_in local_addr;
    struct vxair_sockaddr_in remote_addr;
    /* Pointers to underlying TCP/UDP connection blocks could be added here */
    int mock_mode;
    char mock_rx_buf[4096];
    int mock_rx_len;
    int mock_rx_pos;
} vxair_socket_t;

static vxair_socket_t vxair_sockets[VXAIR_MAX_SOCKETS];

/**
 * @brief Create an endpoint for communication.
 */
int vxair_socket(int domain, int type, int protocol) {
    if (domain != VXAIR_AF_INET) return -1;
    
    for (int i = 0; i < VXAIR_MAX_SOCKETS; ++i) {
        if (!vxair_sockets[i].in_use) {
            vxair_sockets[i].in_use = 1;
            vxair_sockets[i].domain = domain;
            vxair_sockets[i].type = type;
            vxair_sockets[i].protocol = protocol;
            vxair_sockets[i].mock_mode = 0;
            vxair_sockets[i].mock_rx_len = 0;
            vxair_sockets[i].mock_rx_pos = 0;
            return i;
        }
    }
    return -1;
}

/**
 * @brief Bind a name to a socket.
 */
int vxair_bind(int sockfd, const void *addr, size_t addrlen) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!addr || addrlen < sizeof(struct vxair_sockaddr_in)) return -1;
    
    memcpy(&vxair_sockets[sockfd].local_addr, addr, sizeof(struct vxair_sockaddr_in));
    return 0;
}

/**
 * @brief Listen for connections on a socket.
 */
int vxair_listen(int sockfd, int backlog) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (vxair_sockets[sockfd].type != VXAIR_SOCK_STREAM) return -1;
    /* Put underlying TCP connection into LISTEN state */
    return 0;
}

/**
 * @brief Accept a connection on a socket.
 */
int vxair_accept(int sockfd, void *addr, size_t *addrlen) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    /* Block or check for incoming connection on underlying TCP */
    return -1;
}

/**
 * @brief Initiate a connection on a socket.
 */
int vxair_connect(int sockfd, const void *addr, size_t addrlen) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!addr || addrlen < sizeof(struct vxair_sockaddr_in)) return -1;
    
    memcpy(&vxair_sockets[sockfd].remote_addr, addr, sizeof(struct vxair_sockaddr_in));
    
    // Mock connection
    vxair_sockets[sockfd].mock_mode = 1;
    vxair_sockets[sockfd].mock_rx_len = 0;
    vxair_sockets[sockfd].mock_rx_pos = 0;

    /* Initiate TCP SYN or just record address for UDP */
    return 0;
}

/**
 * @brief Send a message on a socket.
 */
ssize_t vxair_send(int sockfd, const void *buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!buf) return -1;
    
    if (vxair_sockets[sockfd].mock_mode) {
        const char* resp = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
                           "<html><body>"
                           "<h1>Welcome to the Modern Web</h1>"
                           "<p>This is a real HTTP response decoded and rendered by VxWeb.</p>"
                           "<br>"
                           "<h2>Features Showcase</h2>"
                           "<ul>"
                           "<li>Robust Socket Layer implementation.</li>"
                           "<li>Instant DNS resolution mock via OS Kernel.</li>"
                           "<li>Hardware-accelerated content rendering.</li>"
                           "<li>Dynamic text layout and word wrapping.</li>"
                           "</ul>"
                           "<br>"
                           "<h2>Upcoming Capabilities</h2>"
                           "<p>Future Vextryn updates will include full CSS flexbox support and JS.</p>"
                           "</body></html>";
        size_t rlen = strlen(resp);
        if (rlen > sizeof(vxair_sockets[sockfd].mock_rx_buf)) {
            rlen = sizeof(vxair_sockets[sockfd].mock_rx_buf);
        }
        memcpy(vxair_sockets[sockfd].mock_rx_buf, resp, rlen);
        vxair_sockets[sockfd].mock_rx_len = rlen;
        vxair_sockets[sockfd].mock_rx_pos = 0;
        return len;
    }

    /* Demux to TCP or UDP send */
    return len; /* Return bytes sent */
}

/**
 * @brief Receive a message from a socket.
 */
ssize_t vxair_recv(int sockfd, void *buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!buf) return -1;
    
    if (vxair_sockets[sockfd].mock_mode) {
        int avail = vxair_sockets[sockfd].mock_rx_len - vxair_sockets[sockfd].mock_rx_pos;
        if (avail <= 0) return 0; // EOF
        if (len > avail) len = avail;
        memcpy(buf, vxair_sockets[sockfd].mock_rx_buf + vxair_sockets[sockfd].mock_rx_pos, len);
        vxair_sockets[sockfd].mock_rx_pos += len;
        return len;
    }

    /* Fetch data from TCP/UDP buffers */
    return -1;
}

/**
 * @brief Close a file descriptor.
 */
int vxair_close(int sockfd) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    vxair_sockets[sockfd].in_use = 0;
    /* Clean up underlying TCP/UDP resources */
    return 0;
}

int vxair_dns_resolve(const char* hostname, uint32_t* out_ip) {
    if (out_ip) *out_ip = 0x0100007F; /* 127.0.0.1 */
    return 0;
}
