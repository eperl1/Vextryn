#ifndef VXAIR_NET_NDP_H
#define VXAIR_NET_NDP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_ndp_header_t
 * @brief NDP message header (ICMPv6).
 */
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t reserved;
    uint8_t target_address[16];
} vxair_ndp_header_t;

/**
 * @brief Initialize NDP neighbor cache.
 */
void vxair_ndp_init(void);

/**
 * @brief Process NDP messages (Router/Neighbor Solicitation/Advertisement).
 * @param packet Pointer to the packet.
 * @param len Packet length.
 */
void vxair_ndp_process_packet(void *packet, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_NDP_H
