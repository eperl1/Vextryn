#include "../include/vxair_syscall.h"
#include "../include/vxair_log.h"

#define MSR_EFER       0xC0000080
#define MSR_STAR       0xC0000081
#define MSR_LSTAR      0xC0000082
#define MSR_FMASK      0xC0000084

extern void vxair_syscall_entry();

static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void vxair_syscall_init(void) {
    vxair_log_info("Syscall: Initializing MSRs for SYSCALL/SYSRET...");
    
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);
    
    uint64_t star = ((uint64_t)0x1B << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star);
    
    wrmsr(MSR_LSTAR, (uint64_t)vxair_syscall_entry);
    
    wrmsr(MSR_FMASK, 0x200);
}

uint64_t vxair_syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (syscall_num) {
        case 0:
            vxair_log_info("User: %s", (char*)arg1);
            return 0;
        case 1:
            // yield
            return 0;
        case 2:
            // exit
            return 0;
        default:
            vxair_log_warn("Unknown syscall %d", (int)syscall_num);
            return -1;
    }
}
