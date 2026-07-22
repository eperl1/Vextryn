#ifndef VXAIR_NET_IP_H
#define VXAIR_NET_IP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_ipv4_header_t
 * @brief IPv4 header structure.
 */
typedef struct __attribute__((packed)) {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
} vxair_ipv4_header_t;

/**
 * @brief Initialize the IP layer.
 */
void vxair_ip_init(void);

/**
 * @brief Receive an IP packet.
 * @param packet Pointer to the raw IP packet buffer.
 * @param len Length of the packet.
 */
void vxair_ip_receive(void *packet, uint16_t len);

/**
 * @brief Send an IP packet.
 * @param dest_ip Destination IP address (as uint32_t pointer).
 * @param protocol Protocol number (e.g., TCP, UDP, ICMP).
 * @param payload Pointer to the payload.
 * @param len Length of the payload.
 * @return 0 on success, -1 on failure.
 */
int vxair_ip_send(const void *dest_ip, uint8_t protocol, const void *payload, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_IP_H
