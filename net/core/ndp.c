#include "ndp.h"
#include <string.h>

#define VXAIR_NDP_CACHE_SIZE 16

typedef struct {
    uint8_t ipv6_addr[16];
    uint8_t mac_addr[6];
    int valid;
} vxair_ndp_entry_t;

static vxair_ndp_entry_t vxair_ndp_cache[VXAIR_NDP_CACHE_SIZE];

/**
 * @brief Initialize NDP neighbor cache.
 */
void vxair_ndp_init(void) {
    for (int i = 0; i < VXAIR_NDP_CACHE_SIZE; ++i) {
        vxair_ndp_cache[i].valid = 0;
    }
}

/**
 * @brief Process NDP messages (Router Solicitation/Advertisement, etc.).
 * @param packet Pointer to the packet.
 * @param len Packet length.
 */
void vxair_ndp_process_packet(void *packet, uint16_t len) {
    if (len < sizeof(vxair_ndp_header_t)) {
        return;
    }
    
    /* vxair_ndp_header_t *ndp = (vxair_ndp_header_t *)packet; */
    /* Parse type (135=NS, 136=NA, 133=RS, 134=RA) and update neighbor cache */
}
