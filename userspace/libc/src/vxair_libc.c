#include "vxair_libc.h"

#define VXAIR_SYSCALL_EXIT   1
#define VXAIR_SYSCALL_READ   3
#define VXAIR_SYSCALL_WRITE  4
#define VXAIR_SYSCALL_OPEN   5
#define VXAIR_SYSCALL_CLOSE  6
#define VXAIR_SYSCALL_WAIT   7
#define VXAIR_SYSCALL_SPAWN  8
#define VXAIR_SYSCALL_MOUNT  21

/**
 * @file vxair_libc.c
 * @brief Minimal C standard library implementation for Vextryn Air OS.
 */

/**
 * @brief System call interface.
 */
int vxair_syscall(int num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    int ret = -1;
#if defined(__x86_64__)
    register uint64_t r10 __asm__("r10") = arg4;
    register uint64_t r8 __asm__("r8") = arg5;
    register uint64_t r9 __asm__("r9") = arg6;
    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (num), "D" (arg1), "S" (arg2), "d" (arg3), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );
#endif
    return ret;
}

// Memory Allocator
#define HEAP_SIZE (1024 * 1024)
static uint8_t heap_memory[HEAP_SIZE];
static size_t heap_offset = 0;

/**
 * @brief Allocate memory.
 *
 * @param size Size in bytes.
 * @return void* Pointer to allocated memory, or NULL on failure.
 */
void* vxair_malloc(size_t size) {
    if (heap_offset + size > HEAP_SIZE) {
        return NULL; // Out of memory
    }
    void* ptr = &heap_memory[heap_offset];
    heap_offset += size;
    return ptr;
}

/**
 * @brief Free allocated memory.
 *
 * @param ptr Pointer to memory to free.
 */
void vxair_free(void* ptr) {
    (void)ptr;
    // Basic bump allocator doesn't support free.
}

/**
 * @brief Fill memory with a constant byte.
 *
 * @param s Pointer to memory.
 * @param c Value to set.
 * @param n Number of bytes.
 * @return void* Pointer to memory.
 */
void* vxair_memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/**
 * @brief Copy memory area.
 *
 * @param dest Destination memory.
 * @param src Source memory.
 * @param n Number of bytes.
 * @return void* Pointer to destination memory.
 */
void* vxair_memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/**
 * @brief Compare two strings.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @return int 0 if equal, non-zero otherwise.
 */
int vxair_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief Compare two strings up to n characters.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param n Number of characters.
 * @return int 0 if equal, non-zero otherwise.
 */
int vxair_strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief Calculate the length of a string.
 *
 * @param s String.
 * @return size_t Length of string.
 */
size_t vxair_strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/**
 * @brief Copy a string.
 *
 * @param dest Destination string.
 * @param src Source string.
 * @return char* Pointer to destination string.
 */
char* vxair_strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * @brief Copy a string up to n characters.
 *
 * @param dest Destination string.
 * @param src Source string.
 * @param n Number of characters.
 * @return char* Pointer to destination string.
 */
char* vxair_strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

/**
 * @brief Locate a character in a string.
 *
 * @param s String.
 * @param c Character to find.
 * @return char* Pointer to character, or NULL if not found.
 */
char* vxair_strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if (c == '\0') return (char*)s;
    return NULL;
}

/**
 * @brief Extract tokens from a string.
 *
 * @param str String to tokenize.
 * @param delim Delimiter characters.
 * @return char* Next token, or NULL.
 */
char* vxair_strtok(char* str, const char* delim) {
    static char* last = NULL;
    if (str == NULL) str = last;
    if (str == NULL) return NULL;
    
    // Skip delimiters
    while (*str && vxair_strchr(delim, *str)) str++;
    if (*str == '\0') {
        last = NULL;
        return NULL;
    }
    
    char* token = str;
    while (*str && !vxair_strchr(delim, *str)) str++;
    if (*str) {
        *str = '\0';
        last = str + 1;
    } else {
        last = NULL;
    }
    
    return token;
}

/**
 * @brief Print a string to standard output without newline.
 *
 * @param str String to print.
 * @return int Number of characters written.
 */
int vxair_print(const char* str) {
    size_t len = vxair_strlen(str);
    return vxair_syscall(VXAIR_SYSCALL_WRITE, 1, (uint64_t)str, len, 0, 0, 0);
}

/**
 * @brief Print a string to standard output with newline.
 *
 * @param str String to print.
 * @return int Number of characters written.
 */
int vxair_puts(const char* str) {
    int written = vxair_print(str);
    written += vxair_print("\n");
    return written;
}

/**
 * @brief Write a single character.
 *
 * @param c Character.
 * @return int Character written.
 */
int vxair_putchar(int c) {
    char ch = (char)c;
    vxair_syscall(VXAIR_SYSCALL_WRITE, 1, (uint64_t)&ch, 1, 0, 0, 0);
    return c;
}

/**
 * @brief Read a single character from standard input.
 *
 * @return int Character read, or -1 on EOF.
 */
int vxair_getchar(void) {
    char ch;
    int ret = vxair_syscall(VXAIR_SYSCALL_READ, 0, (uint64_t)&ch, 1, 0, 0, 0);
    if (ret > 0) return (int)ch;
    return -1;
}

/**
 * @brief Formatted output to standard output.
 *
 * @param format Format string.
 * @param ... Arguments.
 * @return int Characters written.
 */
int vxair_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = 0;
    
    for (const char* p = format; *p; p++) {
        if (*p != '%') {
            vxair_putchar(*p);
            written++;
            continue;
        }
        
        p++; // Skip '%'
        switch (*p) {
            case 's': {
                const char* s = va_arg(args, const char*);
                written += vxair_print(s ? s : "(null)");
                break;
            }
            case 'd': {
                int val = va_arg(args, int);
                char buf[16];
                int i = 0;
                if (val < 0) {
                    vxair_putchar('-');
                    written++;
                    val = -val;
                }
                do {
                    buf[i++] = (char)('0' + (val % 10));
                    val /= 10;
                } while (val > 0);
                while (i > 0) {
                    vxair_putchar(buf[--i]);
                    written++;
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                vxair_putchar(c);
                written++;
                break;
            }
            case '%': {
                vxair_putchar('%');
                written++;
                break;
            }
            default:
                vxair_putchar('%');
                vxair_putchar(*p);
                written += 2;
                break;
        }
    }
    
    va_end(args);
    return written;
}

/**
 * @brief Terminate current process.
 *
 * @param status Exit status.
 */
void vxair_exit(int status) {
    vxair_syscall(VXAIR_SYSCALL_EXIT, status, 0, 0, 0, 0, 0);
    while (1);
}

/**
 * @brief Spawn a new process.
 *
 * @param path Path to executable.
 * @param argv Argument array.
 * @return int PID of new process, or -1 on failure.
 */
int vxair_spawn_process(const char* path, char* const argv[]) {
    return vxair_syscall(VXAIR_SYSCALL_SPAWN, (uint64_t)path, (uint64_t)argv, 0, 0, 0, 0);
}

/**
 * @brief Wait for a child process to change state.
 *
 * @param status Pointer to store status information.
 * @return int PID of child, or -1 on failure.
 */
int vxair_wait(int* status) {
    return vxair_syscall(VXAIR_SYSCALL_WAIT, (uint64_t)status, 0, 0, 0, 0, 0);
}

/**
 * @brief Mount a filesystem.
 *
 * @param fstype Filesystem type.
 * @param target Target directory.
 * @return int 0 on success, -1 on failure.
 */
int vxair_mount(const char* fstype, const char* target) {
    return vxair_syscall(VXAIR_SYSCALL_MOUNT, (uint64_t)fstype, (uint64_t)target, 0, 0, 0, 0);
}

/**
 * @brief Open a file.
 *
 * @param path Path to file.
 * @param flags Open flags.
 * @return int File descriptor, or -1 on failure.
 */
int vxair_open(const char* path, int flags) {
    return vxair_syscall(VXAIR_SYSCALL_OPEN, (uint64_t)path, (uint64_t)flags, 0, 0, 0, 0);
}

/**
 * @brief Close a file descriptor.
 *
 * @param fd File descriptor.
 * @return int 0 on success, -1 on failure.
 */
int vxair_close(int fd) {
    return vxair_syscall(VXAIR_SYSCALL_CLOSE, (uint64_t)fd, 0, 0, 0, 0, 0);
}

/**
 * @brief Read from a file descriptor.
 *
 * @param fd File descriptor.
 * @param buf Buffer.
 * @param count Number of bytes to read.
 * @return int Bytes read, or -1 on failure.
 */
int vxair_read(int fd, void* buf, size_t count) {
    return vxair_syscall(VXAIR_SYSCALL_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)count, 0, 0, 0);
}

/**
 * @brief Write to a file descriptor.
 *
 * @param fd File descriptor.
 * @param buf Buffer.
 * @param count Number of bytes to write.
 * @return int Bytes written, or -1 on failure.
 */
int vxair_write(int fd, const void* buf, size_t count) {
    return vxair_syscall(VXAIR_SYSCALL_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)count, 0, 0, 0);
}
