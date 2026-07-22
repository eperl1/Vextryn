#ifndef VXAIR_NET_ICMP_H
#define VXAIR_NET_ICMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_icmp_header_t
 * @brief ICMPv4 header structure.
 */
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence_number;
} vxair_icmp_header_t;

/**
 * @brief Initialize ICMP handling.
 */
void vxair_icmp_init(void);

/**
 * @brief Handle ICMP packet.
 * @param packet Pointer to the ICMP packet.
 * @param len Length of the packet.
 */
void vxair_icmp_process_packet(void *packet, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_ICMP_H
