#include "ip.h"
#include "arp.h"
#include "ethernet.h"
#include "../../drivers/net/virtio_net.h"
#include <string.h>

static uint32_t vxair_local_ip;
static uint16_t vxair_ip_id = 0;

void vxair_ip_init(void) {
    // QEMU user-mode networking assigns 10.0.2.15
    vxair_local_ip = 0x0F02000A; // 10.0.2.15 in network byte order
}

// Compute RFC 1071 Internet checksum
static uint16_t ip_checksum(const void *data, int len) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len > 0)
        sum += *(const uint8_t *)ptr;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum & 0xFFFF;
}

void vxair_ip_receive(void *packet, uint16_t len) {
    if (len < sizeof(vxair_ipv4_header_t)) {
        return;
    }
    vxair_ipv4_header_t *hdr = (vxair_ipv4_header_t *)packet;
    
    if ((hdr->version & 0xF0) != 0x40) { // Version must be 4
        return;
    }
    
    uint8_t ihl = hdr->ihl & 0x0F;
    (void)ihl;
    
    // Payload starts after IP header
    void *payload = (uint8_t *)packet + sizeof(vxair_ipv4_header_t);
    uint16_t payload_len = len - sizeof(vxair_ipv4_header_t);
    
    // Demultiplex based on protocol
    if (hdr->protocol == 1) {
        // ICMP - not implemented for N1
    } else if (hdr->protocol == 6) {
        // TCP - not implemented for N1
    } else if (hdr->protocol == 17) {
        // UDP - forward to UDP layer
        extern void vxair_udp_receive(void *packet, uint16_t len);
        vxair_udp_receive(payload, payload_len);
    }
}

int vxair_ip_send(const void *dest_ip, uint8_t protocol, const void *payload, uint16_t len) {
    if (!dest_ip || !payload) return -1;
    
    uint8_t buf[512];
    vxair_ipv4_header_t *hdr = (vxair_ipv4_header_t *)buf;
    
    uint16_t total_len = sizeof(vxair_ipv4_header_t) + len;
    if (total_len > 512) return -1;
    
    memset(hdr, 0, sizeof(vxair_ipv4_header_t));
    hdr->version = 4;                // IPv4
    hdr->ihl = 5;                    // Header length = 5 words (20 bytes)
    hdr->tos = 0;
    hdr->total_length = __builtin_bswap16(total_len);
    hdr->identification = __builtin_bswap16(vxair_ip_id++);
    hdr->flags_fragment_offset = __builtin_bswap16(0x4000); // Don't fragment
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->source_ip = vxair_local_ip;
    hdr->dest_ip = *(const uint32_t *)dest_ip;
    
    // Compute checksum (over IP header only)
    hdr->header_checksum = 0;
    hdr->header_checksum = ip_checksum(buf, sizeof(vxair_ipv4_header_t));
    
    memcpy(buf + sizeof(vxair_ipv4_header_t), payload, len);
    
    // Resolve destination MAC via ARP (use gateway for non-local IPs)
    uint32_t dest = *(const uint32_t *)dest_ip;
    uint8_t dest_mac[6];
    
    if (vxair_arp_lookup(dest, dest_mac) != 0) {
        // ARP not resolved yet — this is a simplified driver that assumes
        // the caller has resolved ARP first, OR we use broadcast
        // For DNS, we'll resolve ARP before calling IP send
        return -2;
    }
    
    return vxair_eth_send(dest_mac, 0x0008, buf, total_len);
}
