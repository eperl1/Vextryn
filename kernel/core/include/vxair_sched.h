#pragma once
#include "vxair_types.h"

/**
 * @file vxair_sched.h
 * @brief O(1) Priority Scheduler
 */

typedef enum {
    VXAIR_THREAD_READY,
    VXAIR_THREAD_RUNNING,
    VXAIR_THREAD_BLOCKED,
    VXAIR_THREAD_DEAD
} vxair_thread_state_t;

typedef struct vxair_thread {
    vxair_tid_t tid;
    vxair_pid_t pid;
    vxair_vaddr_t stack_ptr;
    vxair_vaddr_t kernel_stack;
    vxair_thread_state_t state;
    struct vxair_thread* next;
} vxair_thread_t;

void vxair_sched_init(void);
vxair_thread_t* vxair_sched_create_thread(vxair_pid_t pid, void (*entry)(void));
void vxair_sched_yield(void);
void vxair_sched_block(void);
void vxair_sched_unblock(vxair_thread_t* thread);
void vxair_sched_exit(void);
vxair_tid_t vxair_sched_get_current_tid(void);
