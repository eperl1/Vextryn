#include "loopback.h"
#include <string.h>

/**
 * @brief Register loopback interface.
 */
void vxair_loopback_init(void) {
    /* Set up 127.0.0.1 device in the network interface list */
}

/**
 * @brief Queue packet back to the network stack receive path.
 * @param data Packet data.
 * @param len Packet length.
 * @return 0 on success.
 */
int vxair_loopback_send(void *data, uint16_t len) {
    if (!data || len == 0) return -1;
    /* In a real implementation, copy data into an mbuf and push to RX queue */
    return 0;
}
