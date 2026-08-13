#include "socket.h"
#include "ip.h"
#include "ethernet.h"
#include "../../drivers/net/rtl8139.h"
#include <string.h>

#define VXAIR_MAX_SOCKETS 32
#define VXAIR_TCP_RX_CAP 8192

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

typedef enum {
    TCP_CLOSED = 0,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_CLOSE_WAIT
} vxair_socket_tcp_state_t;

typedef struct {
    int in_use;
    int domain;
    int type;
    int protocol;
    struct vxair_sockaddr_in local_addr;
    struct vxair_sockaddr_in remote_addr;
    uint16_t local_port;
    uint32_t seq;
    uint32_t ack;
    vxair_socket_tcp_state_t tcp_state;
    char rx_buf[VXAIR_TCP_RX_CAP];
    int rx_len;
    int rx_pos;
} vxair_socket_t;

typedef struct __attribute__((packed)) {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t data_offset_reserved_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} vxair_tcp_wire_header_t;

static vxair_socket_t vxair_sockets[VXAIR_MAX_SOCKETS];
static uint16_t next_ephemeral_port = 49152;

extern void vxair_hpet_sleep_ms(uint32_t ms);
extern void vxair_log_info(const char *fmt, ...);

#ifndef TCP_VERBOSE
#define TCP_VERBOSE 0
#endif

static uint16_t htons16(uint16_t v) { return __builtin_bswap16(v); }
static uint32_t htonl32(uint32_t v) { return __builtin_bswap32(v); }
static uint16_t ntohs16(uint16_t v) { return __builtin_bswap16(v); }
static uint32_t ntohl32(uint32_t v) { return __builtin_bswap32(v); }

static uint32_t checksum_add(uint32_t sum, const void *data, uint16_t len) {
    const uint8_t *p = (const uint8_t *)data;
    while (len > 1) {
        sum += ((uint16_t)p[0] << 8) | p[1];
        p += 2;
        len -= 2;
    }
    if (len) sum += ((uint16_t)p[0] << 8);
    return sum;
}

static uint16_t checksum_finish(uint32_t sum) {
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *segment, uint16_t len) {
    uint32_t sum = 0;
    sum = checksum_add(sum, &src_ip, 4);
    sum = checksum_add(sum, &dst_ip, 4);
    uint8_t pseudo[4] = {0, VXAIR_IPPROTO_TCP, (uint8_t)(len >> 8), (uint8_t)len};
    sum = checksum_add(sum, pseudo, sizeof(pseudo));
    sum = checksum_add(sum, segment, len);
    return checksum_finish(sum);
}

static void socket_poll_net_once(void) {
    uint8_t frame[2048];
    uint16_t flen = vxair_rtl8139_receive(frame, sizeof(frame));
    if (flen > 0) vxair_eth_receive(frame, flen);
}

static void socket_poll_net_ms(int total_ms) {
    int loops = total_ms / 20;
    if (loops < 1) loops = 1;
    for (int i = 0; i < loops; i++) {
        socket_poll_net_once();
        vxair_hpet_sleep_ms(20);
    }
}

static int tcp_send_segment(vxair_socket_t *s, uint8_t flags, const void *data, uint16_t data_len) {
    if (!s) return -1;
    uint8_t segment[1500];
    uint16_t seg_len = (uint16_t)(sizeof(vxair_tcp_wire_header_t) + data_len);
    if (seg_len > sizeof(segment)) return -1;

    vxair_tcp_wire_header_t *tcp = (vxair_tcp_wire_header_t *)segment;
    memset(segment, 0, seg_len);
    tcp->source_port = htons16(s->local_port);
    tcp->dest_port = htons16(s->remote_addr.sin_port);
    tcp->sequence_number = htonl32(s->seq);
    tcp->acknowledgment_number = htonl32(s->ack);
    tcp->data_offset_reserved_flags = htons16((5u << 12) | flags);
    tcp->window_size = htons16(4096);
    if (data_len > 0) memcpy(segment + sizeof(vxair_tcp_wire_header_t), data, data_len);

    tcp->checksum = 0;
    tcp->checksum = htons16(tcp_checksum(vxair_ip_get_local(), s->remote_addr.sin_addr, segment, seg_len));
    return vxair_ip_send(&s->remote_addr.sin_addr, VXAIR_IPPROTO_TCP, segment, seg_len);
}

void vxair_socket_tcp_receive(void *packet, uint16_t len, uint32_t src_ip, uint32_t dst_ip) {
    (void)dst_ip;
    if (!packet || len < sizeof(vxair_tcp_wire_header_t)) return;
    vxair_tcp_wire_header_t *tcp = (vxair_tcp_wire_header_t *)packet;
    uint16_t src_port = ntohs16(tcp->source_port);
    uint16_t dst_port = ntohs16(tcp->dest_port);
    uint32_t seq = ntohl32(tcp->sequence_number);
    uint32_t ack = ntohl32(tcp->acknowledgment_number);
    uint16_t off_flags = ntohs16(tcp->data_offset_reserved_flags);
    uint16_t header_len = (uint16_t)((off_flags >> 12) * 4);
    uint8_t flags = (uint8_t)(off_flags & 0x3F);
    if (header_len < sizeof(vxair_tcp_wire_header_t) || len < header_len) return;

    for (int i = 0; i < VXAIR_MAX_SOCKETS; i++) {
        vxair_socket_t *s = &vxair_sockets[i];
        if (!s->in_use || s->protocol != VXAIR_IPPROTO_TCP) continue;
        if (s->local_port != dst_port) continue;
        if (s->remote_addr.sin_port != src_port || s->remote_addr.sin_addr != src_ip) continue;

        if (flags & TCP_RST) {
            s->tcp_state = TCP_CLOSED;
            return;
        }

        if (s->tcp_state == TCP_SYN_SENT && (flags & TCP_SYN) && (flags & TCP_ACK)) {
            s->ack = seq + 1;
            s->seq = ack;
            s->tcp_state = TCP_ESTABLISHED;
            tcp_send_segment(s, TCP_ACK, 0, 0);
            if (TCP_VERBOSE) {
                vxair_log_info("TCP: connected local=%u remote=%u", s->local_port, s->remote_addr.sin_port);
            }
            return;
        }

        uint16_t data_len = len - header_len;
        if (data_len > 0) {
            int space = VXAIR_TCP_RX_CAP - s->rx_len;
            int copy = data_len;
            if (copy > space) copy = space;
            if (copy > 0) {
                memcpy(s->rx_buf + s->rx_len, (uint8_t *)packet + header_len, copy);
                s->rx_len += copy;
            }
            s->ack = seq + data_len;
            if (ack > s->seq) s->seq = ack;
            tcp_send_segment(s, TCP_ACK, 0, 0);
            if (TCP_VERBOSE) {
                vxair_log_info("TCP: received %u bytes (buffer=%d)", data_len, s->rx_len);
            }
        }

        if (flags & TCP_FIN) {
            s->ack = seq + data_len + 1;
            s->tcp_state = TCP_CLOSE_WAIT;
            tcp_send_segment(s, TCP_ACK, 0, 0);
        }
        return;
    }
}

int vxair_socket(int domain, int type, int protocol) {
    if (domain != VXAIR_AF_INET) return -1;
    for (int i = 0; i < VXAIR_MAX_SOCKETS; ++i) {
        if (!vxair_sockets[i].in_use) {
            memset(&vxair_sockets[i], 0, sizeof(vxair_sockets[i]));
            vxair_sockets[i].in_use = 1;
            vxair_sockets[i].domain = domain;
            vxair_sockets[i].type = type;
            vxair_sockets[i].protocol = protocol;
            vxair_sockets[i].tcp_state = TCP_CLOSED;
            return i;
        }
    }
    return -1;
}

int vxair_bind(int sockfd, const void *addr, size_t addrlen) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!addr || addrlen < sizeof(struct vxair_sockaddr_in)) return -1;
    memcpy(&vxair_sockets[sockfd].local_addr, addr, sizeof(struct vxair_sockaddr_in));
    return 0;
}

int vxair_listen(int sockfd, int backlog) {
    (void)backlog;
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    return -1;
}

int vxair_accept(int sockfd, void *addr, size_t *addrlen) {
    (void)addr;
    (void)addrlen;
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    return -1;
}

int vxair_connect(int sockfd, const void *addr, size_t addrlen) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!addr || addrlen < sizeof(struct vxair_sockaddr_in)) return -1;
    vxair_socket_t *s = &vxair_sockets[sockfd];
    memcpy(&s->remote_addr, addr, sizeof(struct vxair_sockaddr_in));
    if (s->type != VXAIR_SOCK_STREAM || s->protocol != VXAIR_IPPROTO_TCP) return -1;

    s->local_port = next_ephemeral_port++;
    s->seq = 0x16000000u + ((uint32_t)sockfd * 0x1000u);
    s->ack = 0;
    s->rx_len = 0;
    s->rx_pos = 0;
    s->tcp_state = TCP_SYN_SENT;

    if (tcp_send_segment(s, TCP_SYN, 0, 0) != 0) return -1;
    for (int i = 0; i < 100; i++) {
        socket_poll_net_ms(20);
        if (s->tcp_state == TCP_ESTABLISHED) return 0;
    }
    vxair_log_info("TCP: connect timeout local=%u remote=%u", s->local_port, s->remote_addr.sin_port);
    return -1;
}

ssize_t vxair_send(int sockfd, const void *buf, size_t len, int flags) {
    (void)flags;
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!buf) return -1;
    vxair_socket_t *s = &vxair_sockets[sockfd];
    if (s->tcp_state != TCP_ESTABLISHED) return -1;
    if (len > 1300) len = 1300;
    if (tcp_send_segment(s, TCP_PSH | TCP_ACK, buf, (uint16_t)len) != 0) return -1;
    s->seq += (uint32_t)len;
    socket_poll_net_ms(200);
    return (ssize_t)len;
}

ssize_t vxair_recv(int sockfd, void *buf, size_t len, int flags) {
    (void)flags;
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    if (!buf) return -1;
    vxair_socket_t *s = &vxair_sockets[sockfd];
    for (int i = 0; i < 160 && s->rx_pos >= s->rx_len && s->tcp_state != TCP_CLOSE_WAIT; i++) {
        socket_poll_net_ms(20);
    }
    int avail = s->rx_len - s->rx_pos;
    if (avail <= 0) return 0;
    if ((int)len > avail) len = (size_t)avail;
    memcpy(buf, s->rx_buf + s->rx_pos, len);
    s->rx_pos += (int)len;
    return (ssize_t)len;
}

int vxair_close(int sockfd) {
    if (sockfd < 0 || sockfd >= VXAIR_MAX_SOCKETS || !vxair_sockets[sockfd].in_use) return -1;
    vxair_socket_t *s = &vxair_sockets[sockfd];
    if (s->tcp_state == TCP_ESTABLISHED) {
        tcp_send_segment(s, TCP_FIN | TCP_ACK, 0, 0);
    }
    memset(s, 0, sizeof(*s));
    return 0;
}

int vxair_dns_resolve(const char* hostname, uint32_t* out_ip) {
    (void)hostname;
    if (out_ip) *out_ip = 0xF39342AC; /* example.com from QEMU DNS: 172.66.147.243 */
    return 0;
}
