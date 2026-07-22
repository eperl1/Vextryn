#include "../include/vxair_sched.h"
#include "../include/vxair_log.h"
#include "../include/vxair_kheap.h"

#define SCHED_PRIO_LEVELS 32

typedef struct sched_queue {
    vxair_thread_t* head;
    vxair_thread_t* tail;
} sched_queue_t;

static sched_queue_t active_queues[SCHED_PRIO_LEVELS];
static sched_queue_t expired_queues[SCHED_PRIO_LEVELS];

static uint32_t active_bitmap = 0;
static uint32_t expired_bitmap = 0;

static vxair_thread_t* current_thread = NULL;
static vxair_thread_t* idle_thread = NULL;
static vxair_tid_t next_tid = 1;

static void idle_task(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void vxair_sched_init(void) {
    vxair_log_info("Scheduler: Initializing O(1) Scheduler...");
    for (int i = 0; i < SCHED_PRIO_LEVELS; i++) {
        active_queues[i].head = active_queues[i].tail = NULL;
        expired_queues[i].head = expired_queues[i].tail = NULL;
    }
    
    idle_thread = vxair_sched_create_thread(0, idle_task);
    current_thread = idle_thread;
}

static void enqueue_thread(sched_queue_t* queues, uint32_t* bitmap, vxair_thread_t* thread, int prio) {
    thread->next = NULL;
    if (queues[prio].tail) {
        queues[prio].tail->next = thread;
    } else {
        queues[prio].head = thread;
        *bitmap |= (1 << prio);
    }
    queues[prio].tail = thread;
}

static vxair_thread_t* dequeue_thread(sched_queue_t* queues, uint32_t* bitmap) {
    if (*bitmap == 0) return NULL;
    
    int prio = __builtin_ctz(*bitmap);
    vxair_thread_t* thread = queues[prio].head;
    
    if (thread) {
        queues[prio].head = thread->next;
        if (!queues[prio].head) {
            queues[prio].tail = NULL;
            *bitmap &= ~(1 << prio);
        }
    }
    return thread;
}

vxair_thread_t* vxair_sched_create_thread(vxair_pid_t pid, void (*entry)(void)) {
    vxair_thread_t* thread = (vxair_thread_t*)vxair_kmalloc(sizeof(vxair_thread_t));
    if (!thread) return NULL;
    
    thread->tid = next_tid++;
    thread->pid = pid;
    thread->state = VXAIR_THREAD_READY;
    thread->kernel_stack = (vxair_vaddr_t)vxair_kmalloc(8192);
    thread->stack_ptr = thread->kernel_stack + 8192;
    
    uint64_t* stack = (uint64_t*)thread->stack_ptr;
    *(--stack) = (uint64_t)entry; // RIP
    *(--stack) = 0; // RBP
    *(--stack) = 0; // RBX
    *(--stack) = 0; // R12
    *(--stack) = 0; // R13
    *(--stack) = 0; // R14
    *(--stack) = 0; // R15
    thread->stack_ptr = (vxair_vaddr_t)stack;
    
    enqueue_thread(active_queues, &active_bitmap, thread, 0);
    return thread;
}

extern void vxair_context_switch(vxair_vaddr_t* old_sp, vxair_vaddr_t new_sp);

void vxair_sched_yield(void) {
    vxair_thread_t* prev = current_thread;
    
    if (prev->state == VXAIR_THREAD_RUNNING) {
        prev->state = VXAIR_THREAD_READY;
        enqueue_thread(expired_queues, &expired_bitmap, prev, 0);
    }
    
    if (active_bitmap == 0) {
        if (expired_bitmap == 0) {
            current_thread = idle_thread;
        } else {
            for (int i = 0; i < SCHED_PRIO_LEVELS; i++) {
                active_queues[i] = expired_queues[i];
                expired_queues[i].head = expired_queues[i].tail = NULL;
            }
            active_bitmap = expired_bitmap;
            expired_bitmap = 0;
            current_thread = dequeue_thread(active_queues, &active_bitmap);
        }
    } else {
        current_thread = dequeue_thread(active_queues, &active_bitmap);
    }
    
    if (!current_thread) current_thread = idle_thread;
    
    current_thread->state = VXAIR_THREAD_RUNNING;
    
    if (prev != current_thread) {
        vxair_context_switch(&prev->stack_ptr, current_thread->stack_ptr);
    }
}

void vxair_sched_block(void) {
    if (current_thread) {
        current_thread->state = VXAIR_THREAD_BLOCKED;
        vxair_sched_yield();
    }
}

void vxair_sched_unblock(vxair_thread_t* thread) {
    if (thread && thread->state == VXAIR_THREAD_BLOCKED) {
        thread->state = VXAIR_THREAD_READY;
        enqueue_thread(active_queues, &active_bitmap, thread, 0);
    }
}

void vxair_sched_exit(void) {
    if (current_thread) {
        current_thread->state = VXAIR_THREAD_DEAD;
        vxair_sched_yield();
    }
}

vxair_tid_t vxair_sched_get_current_tid(void) {
    if (current_thread) return current_thread->tid;
    return 0;
}
