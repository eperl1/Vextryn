#include "udp.h"
#include "../core/ip.h"
#include <string.h>

#define VXAIR_MAX_UDP_SOCKETS 16

typedef struct {
    uint16_t local_port;
    uint16_t dest_port;
    uint32_t dest_ip;
    int in_use;
    void (*callback)(const uint8_t *data, uint16_t len, uint32_t src_ip, uint16_t src_port);
} vxair_udp_socket_t;

static vxair_udp_socket_t vxair_udp_sockets[VXAIR_MAX_UDP_SOCKETS];

void vxair_udp_init(void) {
    for (int i = 0; i < VXAIR_MAX_UDP_SOCKETS; ++i) {
        vxair_udp_sockets[i].in_use = 0;
        vxair_udp_sockets[i].local_port = 0;
        vxair_udp_sockets[i].callback = 0;
    }
}

// Register a UDP socket with callback
int vxair_udp_bind(uint16_t local_port,
                   void (*cb)(const uint8_t *data, uint16_t len, uint32_t src_ip, uint16_t src_port)) {
    for (int i = 0; i < VXAIR_MAX_UDP_SOCKETS; ++i) {
        if (!vxair_udp_sockets[i].in_use) {
            vxair_udp_sockets[i].local_port = local_port;
            vxair_udp_sockets[i].callback = cb;
            vxair_udp_sockets[i].in_use = 1;
            return 0;
        }
    }
    return -1;
}

void vxair_udp_receive(void *packet, uint16_t len) {
    if (len < sizeof(vxair_udp_header_t)) {
        return;
    }
    
    vxair_udp_header_t *udp = (vxair_udp_header_t *)packet;
    
    // Find matching socket by destination port
    for (int i = 0; i < VXAIR_MAX_UDP_SOCKETS; ++i) {
        if (vxair_udp_sockets[i].in_use &&
            vxair_udp_sockets[i].local_port == udp->dest_port) {
            
            uint16_t payload_len = len - sizeof(vxair_udp_header_t);
            uint8_t *payload = (uint8_t *)packet + sizeof(vxair_udp_header_t);
            
            if (vxair_udp_sockets[i].callback) {
                vxair_udp_sockets[i].callback(payload, payload_len,
                                              0, udp->source_port); // src_ip TBD from IP layer
            }
            break;
        }
    }
}

int vxair_udp_send(const void *dest_addr, uint16_t dest_port, const void *data, uint16_t len) {
    if (!dest_addr || !data) return -1;
    
    uint8_t buf[512];
    vxair_udp_header_t *udp = (vxair_udp_header_t *)buf;
    
    uint16_t total_len = sizeof(vxair_udp_header_t) + len;
    if (total_len > 512) return -1;
    
    udp->source_port = __builtin_bswap16(12345); // Ephemeral source port
    udp->dest_port = __builtin_bswap16(dest_port);
    udp->length = __builtin_bswap16(total_len);
    udp->checksum = 0; // UDP checksum is optional (0 = no checksum)
    
    memcpy(buf + sizeof(vxair_udp_header_t), data, len);
    
    return vxair_ip_send(dest_addr, 17, buf, total_len);
}
