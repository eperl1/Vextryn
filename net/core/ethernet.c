#include "ethernet.h"
#include <string.h>

/**
 * @brief Initialize Ethernet layer.
 */
void vxair_eth_init(void) {
    /* Set up Ethernet drivers, MAC address configuration, etc. */
}

/**
 * @brief Parse Ethernet header and demux to IP/ARP.
 * @param frame Pointer to frame.
 * @param len Frame length.
 */
void vxair_eth_receive(void *frame, uint16_t len) {
    if (len < sizeof(vxair_eth_header_t)) {
        return;
    }
    vxair_eth_header_t *hdr = (vxair_eth_header_t *)frame;
    
    /* Demux based on ethertype */
    /* 0x0800 = IPv4, 0x0806 = ARP, 0x86DD = IPv6 */
    if (hdr->ethertype == 0x0800) {
        /* Pass to IP layer */
    } else if (hdr->ethertype == 0x0806) {
        /* Pass to ARP layer */
    }
}

/**
 * @brief Construct Ethernet frame and pass to device driver.
 * @param dest_mac Destination MAC.
 * @param ethertype Ethertype.
 * @param payload Payload data.
 * @param len Payload length.
 * @return 0 on success.
 */
int vxair_eth_send(void *dest_mac, uint16_t ethertype, void *payload, uint16_t len) {
    if (!dest_mac || !payload) return -1;
    /* Allocate buffer, construct Ethernet frame, send out via network interface */
    return 0;
}
