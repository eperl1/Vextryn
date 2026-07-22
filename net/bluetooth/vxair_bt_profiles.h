#ifndef VXAIR_BT_PROFILES_H
#define VXAIR_BT_PROFILES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VXAIR_BT_PROFILE_A2DP,
    VXAIR_BT_PROFILE_HFP,
    VXAIR_BT_PROFILE_HID,
    VXAIR_BT_PROFILE_AVRCP,
    VXAIR_BT_PROFILE_SPP
} vxair_bt_profile_type_t;

int vxair_bt_profile_register(vxair_bt_profile_type_t profile);
int vxair_bt_profile_connect(vxair_bt_profile_type_t profile, const uint8_t *mac_addr);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BT_PROFILES_H
