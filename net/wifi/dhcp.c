#include "dhcp.h"
#include <string.h>

/**
 * @file dhcp.c
 * @brief Dynamic Host Configuration Protocol (DHCP) client implementation.
 */

/* Option codes */
#define DHCP_OPT_PAD              0
#define DHCP_OPT_SUBNET_MASK      1
#define DHCP_OPT_ROUTER           3
#define DHCP_OPT_DNS              6
#define DHCP_OPT_REQ_IP           50
#define DHCP_OPT_LEASE_TIME       51
#define DHCP_OPT_MSG_TYPE         53
#define DHCP_OPT_SERVER_ID        54
#define DHCP_OPT_PARAM_REQ_LIST   55
#define DHCP_OPT_END              255

/* Message types */
#define DHCP_MSG_DISCOVER 1
#define DHCP_MSG_OFFER    2
#define DHCP_MSG_REQUEST  3
#define DHCP_MSG_ACK      5
#define DHCP_MSG_NAK      6

static uint32_t vxair_htons(uint16_t v) { return (v >> 8) | (v << 8); }
static uint32_t vxair_htonl(uint32_t v) { return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000); }

int vxair_dhcp_init(vxair_dhcp_context_t *ctx, const uint8_t *mac_addr) {
    if (!ctx || !mac_addr) return -1;
    
    memset(ctx, 0, sizeof(vxair_dhcp_context_t));
    memcpy(ctx->mac_addr, mac_addr, 6);
    ctx->state = VXAIR_DHCP_STATE_INIT;
    
    /* Simulate random XID */
    ctx->xid = 0x12345678;
    
    return 0;
}

static int vxair_dhcp_send_packet(vxair_dhcp_context_t *ctx, uint8_t msg_type) {
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    
    vxair_dhcp_packet_t *pkt = (vxair_dhcp_packet_t *)buffer;
    pkt->op = 1; /* BOOTREQUEST */
    pkt->htype = 1; /* Ethernet */
    pkt->hlen = 6; /* MAC len */
    pkt->xid = vxair_htonl(ctx->xid);
    pkt->flags = vxair_htons(0x8000); /* Broadcast flag */
    memcpy(pkt->chaddr, ctx->mac_addr, 6);
    pkt->magic_cookie = vxair_htonl(0x63825363);
    
    size_t opt_len = 0;
    pkt->options[opt_len++] = DHCP_OPT_MSG_TYPE;
    pkt->options[opt_len++] = 1;
    pkt->options[opt_len++] = msg_type;
    
    if (msg_type == DHCP_MSG_REQUEST) {
        pkt->options[opt_len++] = DHCP_OPT_SERVER_ID;
        pkt->options[opt_len++] = 4;
        uint32_t sid = vxair_htonl(ctx->server_ip);
        memcpy(&pkt->options[opt_len], &sid, 4);
        opt_len += 4;
        
        pkt->options[opt_len++] = DHCP_OPT_REQ_IP;
        pkt->options[opt_len++] = 4;
        uint32_t rip = vxair_htonl(ctx->offered_ip);
        memcpy(&pkt->options[opt_len], &rip, 4);
        opt_len += 4;
    } else if (msg_type == DHCP_MSG_DISCOVER) {
        pkt->options[opt_len++] = DHCP_OPT_PARAM_REQ_LIST;
        pkt->options[opt_len++] = 3;
        pkt->options[opt_len++] = DHCP_OPT_SUBNET_MASK;
        pkt->options[opt_len++] = DHCP_OPT_ROUTER;
        pkt->options[opt_len++] = DHCP_OPT_DNS;
    }
    
    pkt->options[opt_len++] = DHCP_OPT_END;
    
    size_t total_len = sizeof(vxair_dhcp_packet_t) + opt_len;
    
    /* In reality, we'd send this via UDP socket API */
    /* vxair_udp_send(0xFFFFFFFF, VXAIR_DHCP_SERVER_PORT, buffer, total_len); */
    
    return 0;
}

int vxair_dhcp_start(vxair_dhcp_context_t *ctx) {
    if (!ctx) return -1;
    ctx->state = VXAIR_DHCP_STATE_SELECTING;
    return vxair_dhcp_send_packet(ctx, DHCP_MSG_DISCOVER);
}

void vxair_dhcp_receive(vxair_dhcp_context_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || len < sizeof(vxair_dhcp_packet_t)) return;
    
    const vxair_dhcp_packet_t *pkt = (const vxair_dhcp_packet_t *)data;
    
    if (pkt->op != 2) return; /* Not BOOTREPLY */
    if (vxair_htonl(pkt->xid) != ctx->xid) return; /* Mismatched transaction */
    
    uint8_t msg_type = 0;
    
    /* Parse options */
    size_t offset = 0;
    while (offset < len - sizeof(vxair_dhcp_packet_t)) {
        uint8_t opt = pkt->options[offset++];
        if (opt == DHCP_OPT_END) break;
        if (opt == DHCP_OPT_PAD) continue;
        
        uint8_t opt_len = pkt->options[offset++];
        
        switch (opt) {
            case DHCP_OPT_MSG_TYPE:
                msg_type = pkt->options[offset];
                break;
            case DHCP_OPT_SERVER_ID:
                memcpy(&ctx->server_ip, &pkt->options[offset], 4);
                ctx->server_ip = vxair_htonl(ctx->server_ip);
                break;
            case DHCP_OPT_SUBNET_MASK:
                memcpy(&ctx->netmask, &pkt->options[offset], 4);
                ctx->netmask = vxair_htonl(ctx->netmask);
                break;
            case DHCP_OPT_ROUTER:
                memcpy(&ctx->router_ip, &pkt->options[offset], 4);
                ctx->router_ip = vxair_htonl(ctx->router_ip);
                break;
            case DHCP_OPT_DNS:
                memcpy(&ctx->dns_ip, &pkt->options[offset], 4);
                ctx->dns_ip = vxair_htonl(ctx->dns_ip);
                break;
        }
        offset += opt_len;
    }
    
    if (msg_type == DHCP_MSG_OFFER && ctx->state == VXAIR_DHCP_STATE_SELECTING) {
        ctx->offered_ip = vxair_htonl(pkt->yiaddr);
        ctx->state = VXAIR_DHCP_STATE_REQUESTING;
        vxair_dhcp_send_packet(ctx, DHCP_MSG_REQUEST);
    } else if (msg_type == DHCP_MSG_ACK && ctx->state == VXAIR_DHCP_STATE_REQUESTING) {
        ctx->state = VXAIR_DHCP_STATE_BOUND;
        /* IP configuration is now active */
    }
}
