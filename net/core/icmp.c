#include "icmp.h"
#include <string.h>

/**
 * @brief Initialize ICMP handling.
 */
void vxair_icmp_init(void) {
    /* No special initialization required for ICMP */
}

/**
 * @brief Parse ICMP type and code, reply to echo requests.
 * @param packet Pointer to ICMP payload.
 * @param len Payload length.
 */
void vxair_icmp_process_packet(void *packet, uint16_t len) {
    if (len < sizeof(vxair_icmp_header_t)) {
        return;
    }
    vxair_icmp_header_t *icmp = (vxair_icmp_header_t *)packet;
    
    if (icmp->type == 8 && icmp->code == 0) { /* Echo Request */
        /* Convert to Echo Reply */
        icmp->type = 0;
        /* Recalculate checksum in a real implementation */
        icmp->checksum = 0;
    }
}
