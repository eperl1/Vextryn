#include "dns.h"
#include "../udp/udp.h"
#include <string.h>

static uint16_t vxair_htons(uint16_t v) { return __builtin_bswap16(v); }
static uint16_t vxair_ntohs(uint16_t v) { return __builtin_bswap16(v); }

int vxair_dns_init(vxair_dns_context_t *ctx, uint32_t server_ip) {
    if (!ctx) return -1;
    ctx->server_ip = server_ip;
    ctx->current_id = 1000;
    return 0;
}

static size_t format_dns_name(uint8_t *dest, const char *hostname) {
    size_t out_idx = 0;
    const char *start = hostname;
    const char *dot;
    
    while ((dot = strchr(start, '.')) != NULL) {
        size_t len = dot - start;
        dest[out_idx++] = (uint8_t)len;
        memcpy(dest + out_idx, start, len);
        out_idx += len;
        start = dot + 1;
    }
    
    size_t len = strlen(start);
    if (len > 0) {
        dest[out_idx++] = (uint8_t)len;
        memcpy(dest + out_idx, start, len);
        out_idx += len;
    }
    dest[out_idx++] = 0;
    return out_idx;
}

int vxair_dns_query_ipv4(vxair_dns_context_t *ctx, const char *hostname) {
    if (!ctx || !hostname) return -1;
    
    uint8_t buffer[512];
    vxair_dns_header_t *hdr = (vxair_dns_header_t *)buffer;
    
    ctx->current_id++;
    hdr->id = vxair_htons(ctx->current_id);
    hdr->flags = vxair_htons(0x0100);
    hdr->qdcount = vxair_htons(1);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    
    size_t offset = sizeof(vxair_dns_header_t);
    offset += format_dns_name(buffer + offset, hostname);
    
    buffer[offset++] = 0;
    buffer[offset++] = VXAIR_DNS_TYPE_A;
    buffer[offset++] = 0;
    buffer[offset++] = VXAIR_DNS_CLASS_IN;
    
    /* Send via UDP to DNS server port 53 */
    int ret = vxair_udp_send(&ctx->server_ip, VXAIR_DNS_PORT, buffer, offset);
    
    if (ret != 0) return ret;
    return ctx->current_id;
}

int vxair_dns_receive(vxair_dns_context_t *ctx, const uint8_t *data, size_t len, uint32_t *out_ip) {
    if (!ctx || !data || !out_ip || len < sizeof(vxair_dns_header_t)) return -1;
    
    const vxair_dns_header_t *hdr = (const vxair_dns_header_t *)data;
    if (vxair_ntohs(hdr->id) != ctx->current_id) {
        return -2;
    }
    
    uint16_t flags = vxair_ntohs(hdr->flags);
    if ((flags & 0x8000) == 0) {
        return -3;
    }
    if ((flags & 0x000F) != 0) {
        return -4;
    }
    
    uint16_t qdcount = vxair_ntohs(hdr->qdcount);
    uint16_t ancount = vxair_ntohs(hdr->ancount);
    
    if (ancount == 0) {
        return -5;
    }
    
    size_t offset = sizeof(vxair_dns_header_t);
    
    for (int i = 0; i < qdcount; i++) {
        while (offset < len && data[offset] != 0) {
            if ((data[offset] & 0xC0) == 0xC0) {
                offset += 2;
                goto skip_qtype;
            } else {
                offset += data[offset] + 1;
            }
        }
        offset++;
    skip_qtype:
        offset += 4;
    }
    
    for (int i = 0; i < ancount; i++) {
        if (offset >= len) return -6;
        
        if ((data[offset] & 0xC0) == 0xC0) {
            offset += 2;
        } else {
            while (offset < len && data[offset] != 0) {
                offset += data[offset] + 1;
            }
            offset++;
        }
        
        if (offset + 10 > len) return -7;
        
        uint16_t atype = (data[offset] << 8) | data[offset+1];
        uint16_t rdlength = (data[offset+8] << 8) | data[offset+9];
        
        offset += 10;
        
        if (atype == VXAIR_DNS_TYPE_A && rdlength == 4) {
            if (offset + 4 > len) return -8;
            memcpy(out_ip, data + offset, 4);
            return 0;
        }
        
        offset += rdlength;
    }
    
    return -9;
}
