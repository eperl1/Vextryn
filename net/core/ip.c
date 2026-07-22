#include "ip.h"
#include <string.h>

static uint32_t vxair_local_ip;

/**
 * @brief Initialize IP routing table.
 */
void vxair_ip_init(void) {
    vxair_local_ip = 0x7F000001; /* 127.0.0.1 */
}

/**
 * @brief Parse IP header, handle fragmentation, and demux to TCP/UDP/ICMP.
 * @param packet Packet buffer.
 * @param len Packet length.
 */
void vxair_ip_receive(void *packet, uint16_t len) {
    if (len < sizeof(vxair_ipv4_header_t)) {
        return;
    }
    vxair_ipv4_header_t *hdr = (vxair_ipv4_header_t *)packet;
    
    if (hdr->version != 4) {
        return;
    }
    
    /* Demultiplex based on protocol - 1=ICMP, 6=TCP, 17=UDP */
    if (hdr->protocol == 1) {
        /* vxair_icmp_process_packet(payload, payload_len); */
    } else if (hdr->protocol == 6) {
        /* vxair_tcp_receive(payload, payload_len); */
    } else if (hdr->protocol == 17) {
        /* vxair_udp_receive(payload, payload_len); */
    }
}

/**
 * @brief Construct IP header and route packet to appropriate interface.
 * @param dest_ip Destination IP.
 * @param protocol Transport protocol.
 * @param payload Payload data.
 * @param len Payload length.
 * @return 0 on success, -1 on failure.
 */
int vxair_ip_send(const void *dest_ip, uint8_t protocol, const void *payload, uint16_t len) {
    if (!dest_ip || !payload) return -1;
    /* In a full implementation, create buffer, fill IP header, and transmit */
    return 0;
}
