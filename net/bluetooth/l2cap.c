#include "l2cap.h"

/**
 * @file l2cap.c
 * @brief Bluetooth Logical Link Control and Adaptation Protocol (L2CAP) implementation.
 */

int vxair_l2cap_init(void) {
    /* Initialize L2CAP channel management, multiplexing structures */
    return 0;
}

int vxair_l2cap_send(uint16_t handle, uint16_t cid, const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        return -1;
    }
    
    /* Prepend L2CAP header */
    uint8_t buffer[1024]; /* Fixed size for demo */
    if (len + sizeof(vxair_l2cap_hdr_t) > sizeof(buffer)) {
        return -2; /* Payload too large */
    }
    
    vxair_l2cap_hdr_t *hdr = (vxair_l2cap_hdr_t *)buffer;
    hdr->length = (uint16_t)len;
    hdr->cid = cid;
    
    for (size_t i = 0; i < len; i++) {
        buffer[sizeof(vxair_l2cap_hdr_t) + i] = data[i];
    }
    
    /* Forward to HCI layer as ACL data */
    /* vxair_hci_send_acl(handle, buffer, len + sizeof(vxair_l2cap_hdr_t)); */
    
    return len;
}

static void vxair_l2cap_process_signaling(uint16_t handle, const uint8_t *data, size_t len) {
    if (len < sizeof(vxair_l2cap_sig_hdr_t)) {
        return;
    }
    
    const vxair_l2cap_sig_hdr_t *sig_hdr = (const vxair_l2cap_sig_hdr_t *)data;
    
    switch (sig_hdr->code) {
        case 0x01: /* Command Reject */
            break;
        case 0x02: /* Connection Request */
            break;
        case 0x03: /* Connection Response */
            break;
        case 0x04: /* Configuration Request */
            break;
        case 0x05: /* Configuration Response */
            break;
        case 0x06: /* Disconnection Request */
            break;
        case 0x07: /* Disconnection Response */
            break;
        default:
            break;
    }
}

void vxair_l2cap_receive(uint16_t handle, const uint8_t *data, size_t len) {
    if (!data || len < sizeof(vxair_l2cap_hdr_t)) {
        return;
    }
    
    const vxair_l2cap_hdr_t *hdr = (const vxair_l2cap_hdr_t *)data;
    const uint8_t *payload = data + sizeof(vxair_l2cap_hdr_t);
    size_t payload_len = hdr->length;
    
    if (payload_len > len - sizeof(vxair_l2cap_hdr_t)) {
        return; /* Invalid length field */
    }
    
    switch (hdr->cid) {
        case VXAIR_L2CAP_CID_SIGNALING:
        case VXAIR_L2CAP_CID_LE_SIGNALING:
            vxair_l2cap_process_signaling(handle, payload, payload_len);
            break;
        case VXAIR_L2CAP_CID_ATT:
            /* Pass to ATT/GATT layer */
            /* vxair_gatt_receive(handle, payload, payload_len); */
            break;
        case VXAIR_L2CAP_CID_SMP:
            /* Security Manager Protocol */
            break;
        default:
            /* Custom channel */
            break;
    }
}
