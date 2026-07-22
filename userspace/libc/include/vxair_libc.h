#ifndef VXAIR_LIBC_H
#define VXAIR_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file vxair_libc.h
 * @brief Minimal C standard library definitions for Vextryn Air OS
 */

// System call wrappers
int vxair_syscall(int num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// Process management
void vxair_exit(int status);
int vxair_spawn_process(const char* path, char* const argv[]);
int vxair_wait(int* status);

// Filesystem
int vxair_mount(const char* fstype, const char* target);
int vxair_open(const char* path, int flags);
int vxair_close(int fd);
int vxair_read(int fd, void* buf, size_t count);
int vxair_write(int fd, const void* buf, size_t count);

// Memory
void* vxair_malloc(size_t size);
void vxair_free(void* ptr);

// String/Memory Ops
void* vxair_memset(void* s, int c, size_t n);
void* vxair_memcpy(void* dest, const void* src, size_t n);
int vxair_strcmp(const char* s1, const char* s2);
int vxair_strncmp(const char* s1, const char* s2, size_t n);
size_t vxair_strlen(const char* s);
char* vxair_strcpy(char* dest, const char* src);
char* vxair_strncpy(char* dest, const char* src, size_t n);
char* vxair_strchr(const char* s, int c);
char* vxair_strtok(char* str, const char* delim);

// I/O
int vxair_print(const char* str);
int vxair_printf(const char* format, ...);
int vxair_puts(const char* str);
int vxair_putchar(int c);
int vxair_getchar(void);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_LIBC_H
