#ifndef VXAIR_BUS_XHCI_H
#define VXAIR_BUS_XHCI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the xHCI subsystem
 */
void vxair_bus_xhci_init(void);

/**
 * @brief Probe for xHCI controllers on the PCI bus
 */
void vxair_bus_xhci_probe(void);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_BUS_XHCI_H
