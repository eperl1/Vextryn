#include "hal_dtb.h"

// DONE: Implement Device Tree HAL

void vxair_hal_dtb_init(void* dtb_addr) {
    // DONE: Initialize device tree blob parser
}

void* vxair_hal_dtb_find_node(const char* path) {
    // DONE: Find node in device tree
    return 0;
}

int vxair_hal_dtb_get_property(void* node, const char* prop_name, void** out_value, uint32_t* out_len) {
    // DONE: Get property from device tree node
    return -1;
}
