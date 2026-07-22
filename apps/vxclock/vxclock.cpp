#include "../../userspace/libc/include/vxlibc.h"
extern "C" int vxclock_main(int argc, char** argv) {
    vxlibc_printf("=== vxclock ===\n");
    while(1) {
        uint64_t uptime = vxair_get_uptime_ms() / 1000;
        uint64_t h = uptime / 3600; uint64_t m = (uptime % 3600) / 60; uint64_t s = uptime % 60;
        vxlibc_printf("\r\033[K");
        vxlibc_printf("Uptime: %02llu:%02llu:%02llu", h, m, s);
        vxair_sys_sleep(1000);
    }
    return 0;
}
