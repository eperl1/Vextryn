#include "arp.h"
#include <string.h>

#define VXAIR_ARP_TABLE_SIZE 32

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    int valid;
} vxair_arp_entry_t;

static vxair_arp_entry_t vxair_arp_table[VXAIR_ARP_TABLE_SIZE];

/**
 * @brief Initialize ARP table and timers.
 */
void vxair_arp_init(void) {
    for (int i = 0; i < VXAIR_ARP_TABLE_SIZE; ++i) {
        vxair_arp_table[i].valid = 0;
    }
}

/**
 * @brief Handle ARP request/reply.
 * @param packet Pointer to the ARP packet.
 * @param len Length of the packet.
 */
void vxair_arp_process_packet(void *packet, uint16_t len) {
    if (len < sizeof(vxair_arp_header_t)) {
        return;
    }
    vxair_arp_header_t *arp = (vxair_arp_header_t *)packet;
    
    if (arp->opcode == 0x0200) { /* ARP Reply, typically byteswapped in practice */
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
