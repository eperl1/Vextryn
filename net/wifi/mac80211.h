#ifndef VXAIR_NET_WIFI_MAC80211_H
#define VXAIR_NET_WIFI_MAC80211_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file mac80211.h
 * @brief 802.11 MAC layer implementation for Vextryn Air OS.
 */

#define VXAIR_MAC_ADDR_LEN 6

/**
 * @brief 802.11 Frame Control field structure.
 */
typedef struct {
    uint16_t protocol_version : 2;
    uint16_t type : 2;
    uint16_t subtype : 4;
    uint16_t to_ds : 1;
    uint16_t from_ds : 1;
    uint16_t more_frag : 1;
    uint16_t retry : 1;
    uint16_t power_mgt : 1;
    uint16_t more_data : 1;
    uint16_t wep : 1;
    uint16_t order : 1;
} __attribute__((packed)) vxair_mac80211_fc_t;

/**
 * @brief 802.11 MAC Header structure.
 */
typedef struct {
    vxair_mac80211_fc_t fc;
    uint16_t duration;
    uint8_t addr1[VXAIR_MAC_ADDR_LEN];
    uint8_t addr2[VXAIR_MAC_ADDR_LEN];
    uint8_t addr3[VXAIR_MAC_ADDR_LEN];
    uint16_t seq_ctrl;
    uint8_t addr4[VXAIR_MAC_ADDR_LEN]; /* Optional, depends on To DS / From DS */
} __attribute__((packed)) vxair_mac80211_hdr_t;

/**
 * @brief Initialize the 802.11 MAC layer.
 * @return 0 on success, negative error code on failure.
 */
int vxair_mac80211_init(void);

/**
 * @brief Send an 802.11 frame.
 * @param frame Pointer to the frame data.
 * @param length Length of the frame in bytes.
 * @return Number of bytes transmitted, or negative error code.
 */
int vxair_mac80211_send_frame(const uint8_t *frame, size_t length);

/**
 * @brief Handle an incoming 802.11 frame.
 * @param frame Pointer to the received frame data.
 * @param length Length of the received frame.
 */
void vxair_mac80211_receive_frame(const uint8_t *frame, size_t length);

/**
 * @brief Scan for available networks (Active scanning).
 * @return 0 on success.
 */
int vxair_mac80211_scan(void);

#endif /* VXAIR_NET_WIFI_MAC80211_H */
