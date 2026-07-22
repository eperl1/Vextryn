#ifndef VXAIR_NET_BLUETOOTH_L2CAP_H
#define VXAIR_NET_BLUETOOTH_L2CAP_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file l2cap.h
 * @brief Bluetooth Logical Link Control and Adaptation Protocol (L2CAP).
 */

#define VXAIR_L2CAP_CID_SIGNALING       0x0001
#define VXAIR_L2CAP_CID_CONNLESS        0x0002
#define VXAIR_L2CAP_CID_ATT             0x0004
#define VXAIR_L2CAP_CID_LE_SIGNALING    0x0005
#define VXAIR_L2CAP_CID_SMP             0x0006

/**
 * @brief L2CAP Basic Frame Header.
 */
typedef struct {
    uint16_t length;
    uint16_t cid;
} __attribute__((packed)) vxair_l2cap_hdr_t;

/**
 * @brief L2CAP Signaling Command Header.
 */
typedef struct {
    uint8_t  code;
    uint8_t  ident;
    uint16_t length;
} __attribute__((packed)) vxair_l2cap_sig_hdr_t;

/**
 * @brief Initialize L2CAP Layer.
 * @return 0 on success.
 */
int vxair_l2cap_init(void);

/**
 * @brief Send data over L2CAP.
 * @param handle ACL Connection Handle.
 * @param cid Channel Identifier.
 * @param data Data to send.
 * @param len Length of data.
 * @return Number of bytes transmitted, negative on error.
 */
int vxair_l2cap_send(uint16_t handle, uint16_t cid, const uint8_t *data, size_t len);

/**
 * @brief Handle received L2CAP packet.
 * @param handle ACL Connection Handle.
 * @param data Packet data.
 * @param len Packet length.
 */
void vxair_l2cap_receive(uint16_t handle, const uint8_t *data, size_t len);

#endif /* VXAIR_NET_BLUETOOTH_L2CAP_H */
