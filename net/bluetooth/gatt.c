#include "gatt.h"
#include <string.h>

/**
 * @file gatt.c
 * @brief Bluetooth Generic Attribute Profile (GATT) implementation.
 */

#define MAX_ATTRIBUTES 128

static vxair_gatt_attr_t *g_gatt_db[MAX_ATTRIBUTES];
static int g_num_attributes = 0;

int vxair_gatt_init(void) {
    g_num_attributes = 0;
    return 0;
}

int vxair_gatt_register_attribute(vxair_gatt_attr_t *attr) {
    if (!attr || g_num_attributes >= MAX_ATTRIBUTES) {
        return -1;
    }
    g_gatt_db[g_num_attributes++] = attr;
    return 0;
}

static vxair_gatt_attr_t* vxair_gatt_find_attribute(uint16_t handle) {
    for (int i = 0; i < g_num_attributes; i++) {
        if (g_gatt_db[i]->handle == handle) {
            return g_gatt_db[i];
        }
    }
    return NULL;
}

static void vxair_gatt_handle_read_req(uint16_t conn_handle, const uint8_t *data, size_t len) {
    if (len < 3) return;
    
    uint16_t attr_handle = data[1] | (data[2] << 8);
    vxair_gatt_attr_t *attr = vxair_gatt_find_attribute(attr_handle);
    
    uint8_t rsp[256];
    size_t rsp_len = 0;
    
    if (!attr) {
        /* Error Response: Attribute Not Found */
        rsp[0] = VXAIR_ATT_OP_ERROR_RSP;
        rsp[1] = VXAIR_ATT_OP_READ_REQ;
        rsp[2] = data[1];
        rsp[3] = data[2];
        rsp[4] = 0x0A; /* Attribute Not Found */
        rsp_len = 5;
    } else {
        rsp[0] = VXAIR_ATT_OP_READ_RSP;
        size_t copy_len = attr->value_len;
        if (copy_len > 22) copy_len = 22; /* Assume MTU = 23 */
        
        memcpy(rsp + 1, attr->value, copy_len);
        rsp_len = 1 + copy_len;
    }
    
    /* vxair_l2cap_send(conn_handle, VXAIR_L2CAP_CID_ATT, rsp, rsp_len); */
}

void vxair_gatt_receive(uint16_t conn_handle, const uint8_t *data, size_t len) {
    if (!data || len == 0) return;
    
    uint8_t opcode = data[0];
    
    switch (opcode) {
        case VXAIR_ATT_OP_EXCHANGE_MTU_REQ:
            /* Auto-respond with MTU RSP */
            break;
        case VXAIR_ATT_OP_READ_REQ:
            vxair_gatt_handle_read_req(conn_handle, data, len);
            break;
        case VXAIR_ATT_OP_WRITE_REQ:
        case VXAIR_ATT_OP_WRITE_CMD:
            /* Handle write logic */
            break;
        default:
            /* Unsupported opcode, send Error Response */
            break;
    }
}

int vxair_gatt_notify(uint16_t handle, uint16_t attr_handle, const uint8_t *value, size_t len) {
    if (!value || len == 0) return -1;
    
    uint8_t pkt[256];
    if (len > 20) len = 20; /* Basic MTU constraint */
    
    pkt[0] = VXAIR_ATT_OP_HANDLE_VALUE_NOTIF;
    pkt[1] = (attr_handle >> 0) & 0xFF;
    pkt[2] = (attr_handle >> 8) & 0xFF;
    
    memcpy(pkt + 3, value, len);
    
    /* vxair_l2cap_send(handle, VXAIR_L2CAP_CID_ATT, pkt, 3 + len); */
    return 0;
}
