#include "udp.h"
#include <string.h>

#define VXAIR_MAX_UDP_SOCKETS 16

typedef struct {
    uint16_t local_port;
    int in_use;
} vxair_udp_socket_t;

static vxair_udp_socket_t vxair_udp_sockets[VXAIR_MAX_UDP_SOCKETS];

/**
 * @brief Initialize UDP port bindings table.
 */
void vxair_udp_init(void) {
    for (int i = 0; i < VXAIR_MAX_UDP_SOCKETS; ++i) {
        vxair_udp_sockets[i].in_use = 0;
        vxair_udp_sockets[i].local_port = 0;
    }
}

/**
 * @brief Parse UDP header and deliver payload to bound socket.
 * @param packet Pointer to the UDP packet.
 * @param len Length of the packet.
 */
void vxair_udp_receive(void *packet, uint16_t len) {
    if (len < sizeof(vxair_udp_header_t)) {
        return;
    }
    
    vxair_udp_header_t *udp = (vxair_udp_header_t *)packet;
    
    /* Find matching socket */
    for (int i = 0; i < VXAIR_MAX_UDP_SOCKETS; ++i) {
        if (vxair_udp_sockets[i].in_use && vxair_udp_sockets[i].local_port == udp->dest_port) {
            /* Deliver payload to socket */
            break;
        }
    }
}

/**
 * @brief Construct UDP header and pass to IP layer.
 * @param dest_addr Destination address.
 * @param dest_port Destination port.
 * @param data Payload data.
 * @param len Payload length.
 * @return 0 on success, -1 on failure.
 */
int vxair_udp_send(const void *dest_addr, uint16_t dest_port, const void *data, uint16_t len) {
    if (!dest_addr || !data) return -1;
    /* Create UDP header, compute checksum, pass to IP layer */
    return 0;
}
