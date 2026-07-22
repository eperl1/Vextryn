#ifndef VXAIR_NET_LOOPBACK_H
#define VXAIR_NET_LOOPBACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register loopback interface.
 */
void vxair_loopback_init(void);

/**
 * @brief Queue packet back to the network stack receive path.
 * @param data Packet data.
 * @param len Packet length.
 * @return 0 on success.
 */
int vxair_loopback_send(void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_LOOPBACK_H
