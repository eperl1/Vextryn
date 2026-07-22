#ifndef VXAIR_BLE_GATT_H
#define VXAIR_BLE_GATT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t handle;
    uint8_t uuid[16];
    uint8_t properties;
    uint8_t *value;
    uint16_t value_len;
} vxair_gatt_char_t;

int vxair_ble_gatt_init(void);
int vxair_ble_gatt_discover_primary_services(uint16_t conn_handle);
int vxair_ble_gatt_read_char(uint16_t conn_handle, uint16_t char_handle);
int vxair_ble_gatt_write_char(uint16_t conn_handle, uint16_t char_handle, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BLE_GATT_H
