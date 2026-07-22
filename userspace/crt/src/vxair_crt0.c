#include <stdint.h>
#include <stddef.h>
#include "vxair_libc.h"

/**
 * @file vxair_crt0.c
 * @brief C Runtime start entry point.
 */

// Forward declaration of main
extern int main(int argc, char** argv, char** envp);

// Define standard initialization routines (e.g., C++ global constructors)
typedef void (*vxair_init_func_t)(void);
extern vxair_init_func_t __init_array_start[] __attribute__((weak));
extern vxair_init_func_t __init_array_end[] __attribute__((weak));
extern vxair_init_func_t __fini_array_start[] __attribute__((weak));
extern vxair_init_func_t __fini_array_end[] __attribute__((weak));

/**
 * @brief Initialize C library, TLS, and run constructors.
 */
void vxair_init_array(void) {
    if (__init_array_start && __init_array_end) {
        for (vxair_init_func_t* func = __init_array_start; func != __init_array_end; ++func) {
            if (*func) {
                (*func)();
            }
        }
    }
}

/**
 * @brief Run destructors.
 */
void vxair_fini_array(void) {
    if (__fini_array_start && __fini_array_end) {
        for (vxair_init_func_t* func = __fini_array_start; func != __fini_array_end; ++func) {
            if (*func) {
                (*func)();
            }
        }
    }
}

/**
 * @brief C Runtime start entry point.
 * Usually called from assembly, taking arguments from the stack or registers.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param envp Environment vector.
 */
void _start(int argc, char** argv, char** envp) {
    // Initialize C library, TLS, and run constructors
    vxair_init_array();
    
    // Call main
    int ret = main(argc, argv, envp);
    
    // Run destructors
    vxair_fini_array();
    
    // Exit process
    vxair_exit(ret);
}
