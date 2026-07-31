#ifndef VXAIR_BUS_XHCI_H
#define VXAIR_BUS_XHCI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void vxair_bus_xhci_init(void);
void vxair_bus_xhci_probe(void);
void vxair_usb_core_test(void);
int vxair_xhci_configure_endpoint(uint8_t slot_id, uint8_t ep_addr, uint16_t max_packet, uint8_t type);
int vxair_xhci_queue_bulk_trb(uint8_t slot_id, uint8_t ep_addr, void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BUS_XHCI_H
