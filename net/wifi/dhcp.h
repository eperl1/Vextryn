#ifndef VXAIR_NET_WIFI_DHCP_H
#define VXAIR_NET_WIFI_DHCP_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file dhcp.h
 * @brief Dynamic Host Configuration Protocol (DHCP) client for Vextryn Air OS.
 */

#define VXAIR_DHCP_SERVER_PORT 67
#define VXAIR_DHCP_CLIENT_PORT 68

/**
 * @brief DHCP Packet Structure.
 */
typedef struct {
    uint8_t  op;      /* Message op code / message type. 1 = BOOTREQUEST, 2 = BOOTREPLY */
    uint8_t  htype;   /* Hardware address type, e.g., '1' = 10mb ethernet */
    uint8_t  hlen;    /* Hardware address length, e.g., '6' for mac address */
    uint8_t  hops;    /* Client sets to zero, optionally used by relay agents */
    uint32_t xid;     /* Transaction ID */
    uint16_t secs;    /* Seconds elapsed since client began address acquisition or renewal process */
    uint16_t flags;   /* Flags */
    uint32_t ciaddr;  /* Client IP address */
    uint32_t yiaddr;  /* 'your' (client) IP address */
    uint32_t siaddr;  /* IP address of next server to use in bootstrap */
    uint32_t giaddr;  /* Relay agent IP address */
    uint8_t  chaddr[16]; /* Client hardware address */
    uint8_t  sname[64];  /* Optional server host name */
    uint8_t  file[128];  /* Boot file name */
    uint32_t magic_cookie; /* DHCP Magic Cookie (0x63825363) */
    uint8_t  options[0]; /* Optional parameters */
} __attribute__((packed)) vxair_dhcp_packet_t;

/**
 * @brief DHCP Client State.
 */
typedef enum {
    VXAIR_DHCP_STATE_INIT = 0,
    VXAIR_DHCP_STATE_SELECTING,
    VXAIR_DHCP_STATE_REQUESTING,
    VXAIR_DHCP_STATE_BOUND,
    VXAIR_DHCP_STATE_RENEWING,
    VXAIR_DHCP_STATE_REBINDING
} vxair_dhcp_state_t;

/**
 * @brief DHCP Interface Context.
 */
typedef struct {
    vxair_dhcp_state_t state;
    uint32_t xid;
    uint8_t mac_addr[6];
    
    uint32_t offered_ip;
    uint32_t server_ip;
    uint32_t netmask;
    uint32_t router_ip;
    uint32_t dns_ip;
    
    uint32_t lease_time;
    uint32_t t1; /* Renewal time */
    uint32_t t2; /* Rebinding time */
} vxair_dhcp_context_t;

/**
 * @brief Initialize DHCP client context.
 * @param ctx DHCP context.
 * @param mac_addr Hardware MAC address.
 * @return 0 on success.
 */
int vxair_dhcp_init(vxair_dhcp_context_t *ctx, const uint8_t *mac_addr);

/**
 * @brief Start DHCP Discovery process.
 * @param ctx DHCP context.
 * @return 0 on success.
 */
int vxair_dhcp_start(vxair_dhcp_context_t *ctx);

/**
 * @brief Handle received DHCP packet.
 * @param ctx DHCP context.
 * @param data Packet data (starting at DHCP header).
 * @param len Packet length.
 */
void vxair_dhcp_receive(vxair_dhcp_context_t *ctx, const uint8_t *data, size_t len);

#endif /* VXAIR_NET_WIFI_DHCP_H */
