#ifndef VXAIR_NET_BLUETOOTH_HCI_H
#define VXAIR_NET_BLUETOOTH_HCI_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file hci.h
 * @brief Bluetooth Host Controller Interface (HCI) layer.
 */

#define VXAIR_HCI_COMMAND_PKT 0x01
#define VXAIR_HCI_ACLDATA_PKT 0x02
#define VXAIR_HCI_SCODATA_PKT 0x03
#define VXAIR_HCI_EVENT_PKT   0x04

/* OGF (Opcode Group Field) */
#define VXAIR_HCI_OGF_LINK_CTRL    0x01
#define VXAIR_HCI_OGF_LINK_POLICY  0x02
#define VXAIR_HCI_OGF_CONTROLLER   0x03
#define VXAIR_HCI_OGF_INFO_PARAM   0x04
#define VXAIR_HCI_OGF_STATUS_PARAM 0x05
#define VXAIR_HCI_OGF_LE_CTRL      0x08

/* LE OCF (Opcode Command Field) */
#define VXAIR_HCI_OCF_LE_SET_ADV_PARAM   0x0006
#define VXAIR_HCI_OCF_LE_SET_ADV_DATA    0x0008
#define VXAIR_HCI_OCF_LE_SET_ADV_ENABLE  0x000A

/**
 * @brief Construct HCI Opcode from OGF and OCF.
 */
#define VXAIR_HCI_OPCODE(ogf, ocf) (((ogf) << 10) | (ocf))

/**
 * @brief HCI Command Header.
 */
typedef struct {
    uint16_t opcode;
    uint8_t  param_len;
} __attribute__((packed)) vxair_hci_cmd_hdr_t;

/**
 * @brief HCI Event Header.
 */
typedef struct {
    uint8_t  event_code;
    uint8_t  param_len;
} __attribute__((packed)) vxair_hci_event_hdr_t;

/**
 * @brief HCI ACL Data Header.
 */
typedef struct {
    uint16_t handle_flags;
    uint16_t data_len;
} __attribute__((packed)) vxair_hci_acl_hdr_t;

/**
 * @brief Initialize HCI Layer.
 * @return 0 on success.
 */
int vxair_hci_init(void);

/**
 * @brief Send HCI Command.
 * @param ogf Opcode Group Field.
 * @param ocf Opcode Command Field.
 * @param params Command parameters.
 * @param param_len Length of parameters.
 * @return 0 on success.
 */
int vxair_hci_send_command(uint8_t ogf, uint16_t ocf, const uint8_t *params, uint8_t param_len);

/**
 * @brief Handle received HCI packet.
 * @param pkt_type Packet type (Command, Event, ACL, SCO).
 * @param data Packet data.
 * @param len Packet length.
 */
void vxair_hci_receive(uint8_t pkt_type, const uint8_t *data, size_t len);

#endif /* VXAIR_NET_BLUETOOTH_HCI_H */
