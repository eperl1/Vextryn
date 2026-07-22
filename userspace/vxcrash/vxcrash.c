#include "../libc/include/vxlibc.h"
#define CRASH_DIR "/var/log/crash"
#define MAX_CRASHES 99
static void list_crashes(void) {
    int fd = vxair_open(CRASH_DIR, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) { vxlibc_printf("No crash reports found.\n"); return; }
    vxlibc_printf("Crash reports:\n");
    struct vxair_dirent entry; int count = 0;
    while (vxair_getdents(fd, &entry, sizeof(entry)) > 0) {
        if (entry.name[0] == '.') continue;
        vxlibc_printf("  [%d] %s\n", count++, entry.name);
    }
    vxair_close(fd);
    if (count == 0) vxlibc_printf("No crash reports found.\n");
}
static void view_crash(int n) {
    char path[256];
    vxlibc_snprintf(path, sizeof(path), "%s/crash_%04d.txt", CRASH_DIR, n);
    int fd = vxair_open(path, O_RDONLY, 0);
    if (fd < 0) { vxlibc_printf("Crash report %d not found.\n",n); return; }
    char buf[4096]; int bytes;
    while ((bytes = vxair_read(fd, buf, sizeof(buf)-1)) > 0) {
        buf[bytes] = 0; vxlibc_printf("%s", buf);
    }
    vxair_close(fd);
}
static void clear_crashes(void) {
    char cmd[64];
    vxlibc_snprintf(cmd, sizeof(cmd), "rm -f %s/crash_*.txt", CRASH_DIR);
    vxlibc_printf("Crash reports cleared.\n");
}
int main(int argc, char** argv) {
    if (argc < 2) { vxlibc_printf("Usage: vxcrash [list|view N|clear]\n"); return 1; }
    if (vxlibc_strcmp(argv[1], "list") == 0) list_crashes();
    else if (vxlibc_strcmp(argv[1], "view") == 0 && argc > 2) view_crash(vxlibc_atoi(argv[2]));
    else if (vxlibc_strcmp(argv[1], "clear") == 0) clear_crashes();
    else { vxlibc_printf("Unknown command\n"); return 1; }
    return 0;
}
