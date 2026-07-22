#ifndef VXAIR_NET_UDP_H
#define VXAIR_NET_UDP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_udp_header_t
 * @brief UDP header structure.
 */
typedef struct __attribute__((packed)) {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} vxair_udp_header_t;

/**
 * @brief Initialize UDP port bindings table.
 */
void vxair_udp_init(void);

/**
 * @brief Receive UDP packet.
 * @param packet Pointer to the UDP packet.
 * @param len Length of the packet.
 */
void vxair_udp_receive(void *packet, uint16_t len);

/**
 * @brief Send UDP packet.
 * @param dest_addr Destination IP address.
 * @param dest_port Destination port.
 * @param data Pointer to data payload.
 * @param len Length of data.
 * @return 0 on success, -1 on failure.
 */
int vxair_udp_send(const void *dest_addr, uint16_t dest_port, const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_UDP_H
