#include "../include/vxair_log.h"
#include <stdarg.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void vxair_log_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

static void write_serial(char a) {
    while (is_transmit_empty() == 0);
    outb(COM1, a);
}

static void print_string(const char* str) {
    while (*str) write_serial(*str++);
}

static void print_hex(uint64_t num) {
    print_string("0x");
    if (num == 0) { print_string("0"); return; }
    char buf[17]; buf[16] = '\0';
    int i = 15;
    while (num > 0 && i >= 0) {
        int rem = num % 16;
        if (rem < 10) buf[i] = '0' + rem;
        else buf[i] = 'a' + (rem - 10);
        num /= 16; i--;
    }
    print_string(&buf[i + 1]);
}

static void print_dec(int64_t num) {
    if (num == 0) { print_string("0"); return; }
    if (num < 0) { write_serial('-'); num = -num; }
    char buf[20]; buf[19] = '\0';
    int i = 18;
    while (num > 0 && i >= 0) {
        buf[i] = '0' + (num % 10);
        num /= 10; i--;
    }
    print_string(&buf[i + 1]);
}

static void vxair_log_internal(const char* level, const char* fmt, va_list args) {
    print_string("["); print_string(level); print_string("] ");
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 's') print_string(va_arg(args, char*));
            else if (*fmt == 'd') print_dec(va_arg(args, int));
            else if (*fmt == 'x') print_hex(va_arg(args, uint64_t));
            else if (*fmt == 'p') print_hex((uint64_t)va_arg(args, void*));
            else if (*fmt == '%') write_serial('%');
        } else {
            write_serial(*fmt);
        }
        fmt++;
    }
    write_serial('\n');
}

void vxair_log_info(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    vxair_log_internal("INFO", fmt, args); va_end(args);
}
void vxair_log_warn(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    vxair_log_internal("WARN", fmt, args); va_end(args);
}
void vxair_log_error(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    vxair_log_internal("ERROR", fmt, args); va_end(args);
}
