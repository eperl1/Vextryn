#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vxair_libc.h"

/**
 * @file vxair_init.c
 * @brief PID 1 - The INIT System for Vextryn Air OS
 */

/**
 * @brief Main entry point for the Init process.
 * 
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return int Exit status (Init should never really exit).
 */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    vxair_puts("Vextryn Air OS: Init process started.");
    
    // Mount critical filesystems
    vxair_puts("Mounting filesystems...");
    if (vxair_mount("devfs", "/dev") < 0) {
        vxair_puts("Failed to mount /dev");
    }
    if (vxair_mount("procfs", "/proc") < 0) {
        vxair_puts("Failed to mount /proc");
    }
    if (vxair_mount("sysfs", "/sys") < 0) {
        vxair_puts("Failed to mount /sys");
    }
    
    // Start logging daemon or system services
    vxair_puts("Starting syslogd...");
    char* const syslogd_args[] = {"/sbin/syslogd", NULL};
    if (vxair_spawn_process("/sbin/syslogd", syslogd_args) < 0) {
        vxair_puts("Warning: Could not start /sbin/syslogd");
    }
    
    // Start the shell or login process
    vxair_puts("Starting vxsh...");
    char* const shell_args[] = {"/bin/vxsh", NULL};
    if (vxair_spawn_process("/bin/vxsh", shell_args) < 0) {
        vxair_puts("Error: Could not start /bin/vxsh");
    }
    
    // Enter a loop to reap zombie processes (waitpid)
    while (true) {
        int status = 0;
        int pid = vxair_wait(&status);
        if (pid > 0) {
            vxair_printf("Init: Reaped zombie process PID %d with status %d\n", pid, status);
        }
    }
    
    return 0; // Init should never really exit
}
