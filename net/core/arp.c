#include "arp.h"
#include "../../drivers/net/e1000.h"
#include "ethernet.h"
#include <string.h>

#define VXAIR_ARP_TABLE_SIZE 32

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    int valid;
} vxair_arp_entry_t;

static vxair_arp_entry_t vxair_arp_table[VXAIR_ARP_TABLE_SIZE];

void vxair_arp_init(void) {
    for (int i = 0; i < VXAIR_ARP_TABLE_SIZE; ++i) {
        vxair_arp_table[i].valid = 0;
    }
}

// Hardware type: Ethernet = 1
#define ARP_HRD_ETHER 0x0100
// Protocol: IPv4 = 0x0800
#define ARP_PRO_IPV4  0x0008
// Opcodes
#define ARP_OP_REQUEST 0x0100
#define ARP_OP_REPLY   0x0200

// Broadcast MAC
static const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Look up a MAC in the ARP table
int vxair_arp_lookup(uint32_t ip, uint8_t *out_mac) {
    for (int i = 0; i < VXAIR_ARP_TABLE_SIZE; i++) {
        if (vxair_arp_table[i].valid && vxair_arp_table[i].ip == ip) {
            memcpy(out_mac, vxair_arp_table[i].mac, 6);
            return 0;
        }
    }
    return -1; // Not found
}

// Send an ARP request for the given IP
int vxair_arp_request(uint32_t target_ip) {
    vxair_arp_header_t arp_req;
    
    arp_req.hardware_type = ARP_HRD_ETHER;
    arp_req.protocol_type = ARP_PRO_IPV4;
    arp_req.hardware_len = 6;
    arp_req.protocol_len = 4;
    arp_req.opcode = ARP_OP_REQUEST;
    memcpy(arp_req.sender_mac, g_e1000.mac_addr, 6);
    arp_req.sender_ip = 0x0F02000A; // 10.0.2.15 in network byte order
    memset(arp_req.target_mac, 0, 6);
    arp_req.target_ip = target_ip;
    
    return vxair_eth_send((void *)broadcast_mac, 0x0608, &arp_req, sizeof(vxair_arp_header_t));
}

void vxair_arp_process_packet(void *packet, uint16_t len) {
    if (len < sizeof(vxair_arp_header_t)) {
        return;
    }
    vxair_arp_header_t *arp = (vxair_arp_header_t *)packet;
    
    if (arp->opcode == ARP_OP_REPLY) {
        for (int i = 0; i < VXAIR_ARP_TABLE_SIZE; ++i) {
            if (!vxair_arp_table[i].valid || vxair_arp_table[i].ip == arp->sender_ip) {
                vxair_arp_table[i].ip = arp->sender_ip;
                memcpy(vxair_arp_table[i].mac, arp->sender_mac, 6);
                vxair_arp_table[i].valid = 1;
                break;
            }
        }
    }
}
