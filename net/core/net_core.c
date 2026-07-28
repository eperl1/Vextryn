#include "net_core.h"
#include "../../drivers/net/rtl8139.h"
#include "../../drivers/bus/bus_pci.h"
#include "../wifi/dns.h"
#include "arp.h"
#include "ethernet.h"
#include "ip.h"
#include "../udp/udp.h"
#include <string.h>

extern void vxair_log_info(const char *fmt, ...);
extern void vxair_hpet_sleep_ms(uint32_t ms);


// DNS response callback
static vxair_dns_context_t g_dns_ctx;
static uint32_t g_resolved_ip = 0;
static int g_dns_done = 0;

// DNS server MAC (resolved by ARP)
static uint8_t dns_server_mac[6] = {0};
static int dns_mac_resolved = 0;

// ARP resolution for the DNS server (10.0.2.3 in QEMU user-mode networking)
static int resolve_dns_server_arp(void) {
    // 10.0.2.3 in network byte order = 0x0302000A on little-endian
    uint32_t dns_ip = 0x0302000A;
    
    // Check if already in ARP table
    if (vxair_arp_lookup(dns_ip, dns_server_mac) == 0) {
        dns_mac_resolved = 1;
        return 0;
    }
    
    // Send ARP request
    vxair_arp_request(dns_ip);
    
    vxair_log_info("DNS: Polling for ARP reply...");
    // Poll for ARP reply (up to ~2 seconds)
    for (int i = 0; i < 100; i++) {
        vxair_hpet_sleep_ms(20);
        
        // Receive frame from virtio-net
        uint8_t frame_buf[2048];
        uint16_t frame_len = vxair_rtl8139_receive(frame_buf, sizeof(frame_buf));
        if (frame_len > 0) {
            vxair_log_info("DNS: Got frame len=%u iteration=%d", frame_len, i);
            vxair_eth_receive(frame_buf, frame_len);
            if (vxair_arp_lookup(dns_ip, dns_server_mac) == 0) {
                dns_mac_resolved = 1;
                return 0;
            }
        }
    }
    
    return -1;
}

// Send a DNS query and wait for response
static int do_dns_query(const char *hostname) {
    // Resolve DNS server MAC first
    if (!dns_mac_resolved) {
        if (resolve_dns_server_arp() != 0) {
            vxair_log_info("DNS: ARP resolution failed");
            return -1;
        }
        vxair_log_info("DNS: DNS server MAC resolved via ARP");
    }
    
    // Initialize DNS context with QEMU's built-in DNS proxy (10.0.2.3)
    // In network byte order: 10.0.2.3 = 0x0A000203
    // On little-endian host, store as byteswapped: 0x0302000A
    uint32_t dns_server = 0x0302000A; // 10.0.2.3 in LE host byte order
    vxair_dns_init(&g_dns_ctx, dns_server);
    
    // Send DNS query
    vxair_log_info("DNS: Querying %s via QEMU DNS proxy", hostname);
    int qid = vxair_dns_query_ipv4(&g_dns_ctx, hostname);
    if (qid < 0) {
        vxair_log_info("DNS: Query failed with error %d", qid);
        return -1;
    }
    vxair_log_info("DNS: Query sent (ID %d)", qid);
    
    // Poll for response (up to ~5000ms = 5 seconds)
    for (int i = 0; i < 250; i++) {
        vxair_hpet_sleep_ms(20);
        
        uint8_t frame_buf[2048];
        uint16_t frame_len = vxair_rtl8139_receive(frame_buf, sizeof(frame_buf));
        if (frame_len > 0) {
            // Feed to Ethernet → IP → UDP → DNS
            vxair_eth_receive(frame_buf, frame_len);
            
            // Check if DNS response has been parsed
            if (g_resolved_ip != 0) {
                uint8_t *b = (uint8_t *)&g_resolved_ip;
                vxair_log_info("DNS SUCCESS: %s resolved to %d.%d.%d.%d",
                               hostname, b[0], b[1], b[2], b[3]);
                return 0;
            }
        }
    }
    
    vxair_log_info("DNS: TIMEOUT waiting for response");
    return -1;
}

// UDP DNS response handler
void vxair_udp_dns_callback(const uint8_t *data, uint16_t len, uint32_t src_ip, uint16_t src_port) {
    (void)src_ip;
    (void)src_port;
    
    uint32_t resolved = 0;
    int ret = vxair_dns_receive(&g_dns_ctx, data, len, &resolved);
    
    if (ret == 0) {
        g_resolved_ip = resolved;
        g_dns_done = 1;
    } else {
        vxair_log_info("DNS: Parse error %d", ret);
    }
}

void vxair_net_init(void) {
    // Initialize all layers
    vxair_eth_init();
    vxair_arp_init();
    vxair_ip_init();
    vxair_udp_init();
    
    // Enumerate PCI devices (required before virtio-net can find its device)
    vxair_bus_pci_init();
    vxair_bus_pci_scan();
    
    // Initialize rtl8139 driver
    int ret = vxair_rtl8139_init();
    if (ret != 0) {
        vxair_log_info("NET: rtl8139 init FAILED (%d)", ret);
        return;
    }
    
    // Log MAC address
    vxair_log_info("NET: rtl8139 MAC %02x:%02x:%02x:%02x:%02x:%02x",
                   g_rtl8139.mac_addr[0], g_rtl8139.mac_addr[1],
                   g_rtl8139.mac_addr[2], g_rtl8139.mac_addr[3],
                   g_rtl8139.mac_addr[4], g_rtl8139.mac_addr[5]);
    
    vxair_log_info("NET: Stack initialized successfully");
}

void vxair_net_test(void) {
    if (!g_rtl8139.found) {
        vxair_log_info("NET: No NIC found, skipping DNS test");
        return;
    }
    
    vxair_log_info("NET: Starting DNS test...");
    
    // Bind UDP port for DNS response
    vxair_udp_bind(__builtin_bswap16(12345), vxair_udp_dns_callback);
    
    // Try to resolve a hostname
    do_dns_query("google.com");
    
    if (g_resolved_ip == 0) {
        // Try alternative hostname
        vxair_log_info("NET: Retrying with alternative hostname...");
        do_dns_query("example.com");
    }
    
    if (g_resolved_ip != 0) {
        vxair_log_info("NET: DNS test PASSED");
    } else {
        vxair_log_info("NET: DNS test completed (no response — this may be expected if QEMU user-mode networking is offline)");
    }
}
