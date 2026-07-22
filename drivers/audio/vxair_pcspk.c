// #include "../../kernel/hal/vxair_hpet.h"
void vxair_hpet_sleep_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 1000; i++);
}

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t val; __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port)); return val; }
void vxair_beep(uint32_t freq, uint32_t ms) {
    if (freq == 0) { outb(inb(0x61) & ~3, 0x61); return; }
    uint32_t div = 1193182 / freq;
    outb(0xB6, 0x43); outb((uint8_t)(div & 0xFF), 0x42); outb((uint8_t)(div >> 8), 0x42);
    outb(inb(0x61) | 3, 0x61);
    vxair_hpet_sleep_ms(ms);
    outb(inb(0x61) & ~3, 0x61);
}
void vxair_play_boot_chime(void) {
    vxair_beep(523, 80); vxair_hpet_sleep_ms(20);
    vxair_beep(659, 80); vxair_hpet_sleep_ms(20);
    vxair_beep(784, 80); vxair_hpet_sleep_ms(20);
    vxair_beep(1047, 150); vxair_hpet_sleep_ms(50);
}
void vxair_sound_error(void) { for (int i = 0; i < 3; i++) { vxair_beep(440, 80); vxair_hpet_sleep_ms(50); } }
void vxair_sound_notify(void) { vxair_beep(880, 60); vxair_hpet_sleep_ms(20); vxair_beep(1047, 60); }
void vxair_sound_click(void) { vxair_beep(1200, 20); }
