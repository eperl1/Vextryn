#ifndef VXAIR_NET_TCP_H
#define VXAIR_NET_TCP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_tcp_header_t
 * @brief TCP header structure.
 */
typedef struct __attribute__((packed)) {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t data_offset_reserved_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} vxair_tcp_header_t;

/**
 * @brief Initialize TCP state machines and timers.
 */
void vxair_tcp_init(void);

/**
 * @brief Receive TCP packet.
 * @param packet Pointer to the TCP segment.
 * @param len Length of the segment.
 */
void vxair_tcp_receive(void *packet, uint16_t len);

/**
 * @brief Send TCP packet.
 * @param connection Pointer to the connection state structure.
 * @param data Pointer to data payload.
 * @param len Length of data.
 * @return Number of bytes sent or -1 on error.
 */
int vxair_tcp_send(void *connection, const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_TCP_H
