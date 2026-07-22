#ifndef VXAIR_NET_WIFI_DNS_H
#define VXAIR_NET_WIFI_DNS_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file dns.h
 * @brief Domain Name System (DNS) resolver for Vextryn Air OS.
 */

#define VXAIR_DNS_PORT 53

/**
 * @brief DNS Header Structure.
 */
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) vxair_dns_header_t;

/**
 * @brief DNS Query Type.
 */
#define VXAIR_DNS_TYPE_A     1
#define VXAIR_DNS_TYPE_AAAA  28

/**
 * @brief DNS Class.
 */
#define VXAIR_DNS_CLASS_IN   1

/**
 * @brief DNS Resolver Context.
 */
typedef struct {
    uint32_t server_ip;
    uint16_t current_id;
} vxair_dns_context_t;

/**
 * @brief Initialize DNS resolver.
 * @param ctx DNS context.
 * @param server_ip IPv4 address of DNS server.
 * @return 0 on success.
 */
int vxair_dns_init(vxair_dns_context_t *ctx, uint32_t server_ip);

/**
 * @brief Send a DNS A-record query.
 * @param ctx DNS context.
 * @param hostname Hostname to resolve.
 * @return Transaction ID on success, negative error code on failure.
 */
int vxair_dns_query_ipv4(vxair_dns_context_t *ctx, const char *hostname);

/**
 * @brief Handle received DNS response.
 * @param ctx DNS context.
 * @param data Packet data.
 * @param len Packet length.
 * @param out_ip Pointer to store resolved IP address.
 * @return 0 on success, negative error code on failure.
 */
int vxair_dns_receive(vxair_dns_context_t *ctx, const uint8_t *data, size_t len, uint32_t *out_ip);

#endif /* VXAIR_NET_WIFI_DNS_H */
