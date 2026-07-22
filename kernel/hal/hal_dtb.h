#ifndef VXAIR_HAL_DTB_H
#define VXAIR_HAL_DTB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void vxair_hal_dtb_init(void* dtb_addr);
void* vxair_hal_dtb_find_node(const char* path);
int vxair_hal_dtb_get_property(void* node, const char* prop_name, void** out_value, uint32_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_HAL_DTB_H
