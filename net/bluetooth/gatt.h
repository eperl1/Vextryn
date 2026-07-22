#ifndef VXAIR_NET_BLUETOOTH_GATT_H
#define VXAIR_NET_BLUETOOTH_GATT_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file gatt.h
 * @brief Bluetooth Generic Attribute Profile (GATT) and ATT protocol layer.
 */

/* ATT Opcodes */
#define VXAIR_ATT_OP_ERROR_RSP             0x01
#define VXAIR_ATT_OP_EXCHANGE_MTU_REQ      0x02
#define VXAIR_ATT_OP_EXCHANGE_MTU_RSP      0x03
#define VXAIR_ATT_OP_FIND_INFO_REQ         0x04
#define VXAIR_ATT_OP_FIND_INFO_RSP         0x05
#define VXAIR_ATT_OP_READ_BY_TYPE_REQ      0x08
#define VXAIR_ATT_OP_READ_BY_TYPE_RSP      0x09
#define VXAIR_ATT_OP_READ_REQ              0x0A
#define VXAIR_ATT_OP_READ_RSP              0x0B
#define VXAIR_ATT_OP_WRITE_REQ             0x12
#define VXAIR_ATT_OP_WRITE_RSP             0x13
#define VXAIR_ATT_OP_WRITE_CMD             0x52
#define VXAIR_ATT_OP_HANDLE_VALUE_NOTIF    0x1B
#define VXAIR_ATT_OP_HANDLE_VALUE_IND      0x1D

/**
 * @brief GATT Attribute representation.
 */
typedef struct {
    uint16_t handle;
    uint16_t uuid16; /* Simplification: 16-bit UUIDs only */
    uint8_t  properties;
    uint8_t  *value;
    uint16_t value_len;
} vxair_gatt_attr_t;

/**
 * @brief Initialize GATT layer.
 * @return 0 on success.
 */
int vxair_gatt_init(void);

/**
 * @brief Register a GATT attribute in the local database.
 * @param attr Pointer to attribute structure.
 * @return 0 on success.
 */
int vxair_gatt_register_attribute(vxair_gatt_attr_t *attr);

/**
 * @brief Handle received ATT packet.
 * @param handle ACL Connection Handle.
 * @param data ATT packet data.
 * @param len Packet length.
 */
void vxair_gatt_receive(uint16_t handle, const uint8_t *data, size_t len);

/**
 * @brief Send Handle Value Notification to peer.
 * @param handle ACL Connection Handle.
 * @param attr_handle Attribute Handle.
 * @param value Value to notify.
 * @param len Length of value.
 * @return 0 on success.
 */
int vxair_gatt_notify(uint16_t handle, uint16_t attr_handle, const uint8_t *value, size_t len);

#endif /* VXAIR_NET_BLUETOOTH_GATT_H */
