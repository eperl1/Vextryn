#include "ethernet.h"
#include "../../drivers/net/rtl8139.h"
#include <string.h>

/**
 * @brief Initialize Ethernet layer.
 */
void vxair_eth_init(void) {
    /* Ethernet init is handled by virtio-net driver init */
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
    
    /* Demux based on ethertype (network byte order) */
    /* 0x0800 = IPv4, 0x0806 = ARP */
    if (hdr->ethertype == 0x0008) { /* 0x0800 in net byte order = hosts byteswapped */
        /* Pass to IP layer (payload after Ethernet header) */
        extern void vxair_ip_receive(void *packet, uint16_t len);
        vxair_ip_receive((uint8_t *)frame + sizeof(vxair_eth_header_t),
                         len - sizeof(vxair_eth_header_t));
    } else if (hdr->ethertype == 0x0608) { /* 0x0806 in net byte order */
        /* Pass to ARP layer */
        extern void vxair_arp_process_packet(void *packet, uint16_t len);
        vxair_arp_process_packet((uint8_t *)frame + sizeof(vxair_eth_header_t),
                                 len - sizeof(vxair_eth_header_t));
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
    if (!dest_mac || !payload || !g_rtl8139.found) return -1;
    
    uint16_t frame_len = sizeof(vxair_eth_header_t) + len;
    if (frame_len > RTL8139_MAX_FRAME_SIZE) return -1;
    
    uint8_t frame[RTL8139_MAX_FRAME_SIZE];
    vxair_eth_header_t *hdr = (vxair_eth_header_t *)frame;
    
    memcpy(hdr->dest_mac, dest_mac, 6);
    memcpy(hdr->src_mac, g_rtl8139.mac_addr, 6);
    hdr->ethertype = ethertype; /* Already in network byte order */
    memcpy(frame + sizeof(vxair_eth_header_t), payload, len);

    // Pad to minimum Ethernet frame size (60 bytes payload)
    if (frame_len < RTL8139_MIN_FRAME_SIZE) {
        memset(frame + frame_len, 0, RTL8139_MIN_FRAME_SIZE - frame_len);
        frame_len = RTL8139_MIN_FRAME_SIZE;
    }

    return vxair_rtl8139_send(frame, frame_len);
}
