#ifndef VXAIR_NET_CORE_H
#define VXAIR_NET_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_net_device_t
 * @brief Represents a network interface device.
 */
typedef struct {
    char name[16];
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    bool is_up;
} vxair_net_device_t;

/**
 * @brief Initialize networking stack subsystems.
 */
void vxair_net_init(void);

/**
 * @brief Handle incoming packet, demux to ethernet, loopback, etc.
 * @param data Packet data buffer.
 * @param len Packet length.
 */
void vxair_net_receive_packet(void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_CORE_H
