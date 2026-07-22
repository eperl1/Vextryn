#ifndef VXAIR_NET_ARP_H
#define VXAIR_NET_ARP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_arp_header_t
 * @brief ARP header structure.
 */
typedef struct __attribute__((packed)) {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} vxair_arp_header_t;

/**
 * @brief Initialize ARP table and timers.
 */
void vxair_arp_init(void);

/**
 * @brief Handle ARP request/reply.
 * @param packet Pointer to the ARP packet.
 * @param len Length of the packet.
 */
void vxair_arp_process_packet(void *packet, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_ARP_H
