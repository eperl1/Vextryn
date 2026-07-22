#pragma once
#include <stdint.h>
#include <stddef.h>

// Forward declare boot info
typedef struct vxair_boot_info vxair_boot_info_t;

void vxair_fb_init(vxair_boot_info_t* info);
void vxair_fb_put_pixel(int32_t x, int32_t y,
                         uint32_t color);
void vxair_fb_fill_rect(int32_t x, int32_t y,
                          int32_t w, int32_t h,
                          uint32_t color);
void vxair_fb_blit(const uint32_t* src,
                    int32_t dst_x, int32_t dst_y,
                    int32_t w, int32_t h);
void vxair_fb_flip(void);
void vxair_fb_clear(uint32_t color);
void vxair_fb_test(void);
void vxair_font_draw_char(uint32_t x, uint32_t y,
                            char ch,
                            uint32_t fg, uint32_t bg);
void vxair_font_draw_string(uint32_t x, uint32_t y,
                              const char* str,
                              uint32_t fg, uint32_t bg);
uint32_t vxair_fb_get_width(void);
uint32_t vxair_fb_get_height(void);
