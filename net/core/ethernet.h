#ifndef VXAIR_NET_ETHERNET_H
#define VXAIR_NET_ETHERNET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_eth_header_t
 * @brief Ethernet frame header.
 */
typedef struct __attribute__((packed)) {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} vxair_eth_header_t;

/**
 * @brief Initialize Ethernet layer.
 */
void vxair_eth_init(void);

/**
 * @brief Receive an Ethernet frame.
 * @param frame Pointer to the frame data.
 * @param len Length of the frame.
 */
void vxair_eth_receive(void *frame, uint16_t len);

/**
 * @brief Send an Ethernet frame.
 * @param dest_mac Destination MAC address.
 * @param ethertype Ethertype field.
 * @param payload Pointer to payload.
 * @param len Length of payload.
 * @return 0 on success, -1 on failure.
 */
int vxair_eth_send(void *dest_mac, uint16_t ethertype, void *payload, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_ETHERNET_H
