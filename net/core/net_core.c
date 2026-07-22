#include "net_core.h"
#include <string.h>

/**
 * @brief Initialize networking stack subsystems.
 */
void vxair_net_init(void) {
    /* Initialize memory pools, timers, and interfaces */
}

/**
 * @brief Handle incoming packet, demux to ethernet, loopback, etc.
 * @param data Packet data buffer.
 * @param len Packet length.
 */
void vxair_net_receive_packet(void *data, size_t len) {
    if (!data || len == 0) return;
    
    /* Demux to Ethernet or loopback based on device source */
    /* Currently assuming Ethernet reception */
}
