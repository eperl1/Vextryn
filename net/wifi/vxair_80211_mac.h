#ifndef VXAIR_NET_WIFI_80211_MAC_H
#define VXAIR_NET_WIFI_80211_MAC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VXAIR_MAC_STATE_IDLE,
    VXAIR_MAC_STATE_SCANNING,
    VXAIR_MAC_STATE_AUTHENTICATING,
    VXAIR_MAC_STATE_ASSOCIATING,
    VXAIR_MAC_STATE_ASSOCIATED
} vxair_mac_state_t;

typedef struct {
    uint8_t local_mac[6];
    uint8_t bssid[6];
    vxair_mac_state_t state;
    bool encryption_enabled;
    /* TX/RX Queues would go here */
} vxair_80211_mac_t;

void vxair_80211_init(vxair_80211_mac_t *mac_ctx);
void vxair_80211_receive_frame(vxair_80211_mac_t *mac_ctx, void *frame, uint16_t len);
int vxair_80211_send_frame(vxair_80211_mac_t *mac_ctx, void *payload, uint16_t len, uint16_t type);
void vxair_80211_scan(vxair_80211_mac_t *mac_ctx, bool active);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_WIFI_80211_MAC_H
