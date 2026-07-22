#include "../../userspace/libc/include/vxlibc.h"
extern "C" int vxsysmon_main(int argc, char** argv) {
    vxlibc_printf("=== System Monitor ===\n");
    vxlibc_printf("%-6s %-16s %-8s\n", "PID", "NAME", "STATE");
    vxlibc_printf("%-6s %-16s %-8s\n", "---", "----", "-----");
    struct vxair_proc_info { int pid; char name[16]; char state[8]; } procs[64];
    int count = vxair_ioctl(0, 0x5001, procs);
    if (count < 0) count = 0;
    for (int i = 0; i < count; i++) {
        vxlibc_printf("%-6d %-16s %-8s\n", procs[i].pid, procs[i].name, procs[i].state);
    }
    vxlibc_printf("\nMemory:\n");
    int fd = vxair_open("/proc/meminfo", O_RDONLY, 0);
    if (fd >= 0) {
        char buf[512];
        int n = vxair_read(fd, buf, sizeof(buf)-1);
        if (n > 0) { buf[n] = 0; vxlibc_printf("%s\n", buf); }
        vxair_close(fd);
    }
    return 0;
}
