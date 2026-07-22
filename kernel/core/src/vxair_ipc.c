#include "../include/vxair_ipc.h"
#include "../include/vxair_sched.h"
#include "../include/vxair_log.h"
#include "../include/vxair_kheap.h"

typedef struct ipc_queue_node {
    vxair_msg_t msg;
    struct ipc_queue_node* next;
} ipc_queue_node_t;

static ipc_queue_node_t* thread_queues[1024];

void vxair_ipc_init(void) {
    vxair_log_info("IPC: Initializing message queues...");
    for (int i = 0; i < 1024; i++) thread_queues[i] = NULL;
}

bool vxair_ipc_send(vxair_tid_t dest, vxair_msg_t* msg) {
    if (dest >= 1024) return false;
    
    ipc_queue_node_t* node = (ipc_queue_node_t*)vxair_kmalloc(sizeof(ipc_queue_node_t));
    if (!node) return false;
    
    node->msg = *msg;
    node->msg.sender = vxair_sched_get_current_tid();
    node->next = NULL;
    
    if (!thread_queues[dest]) {
        thread_queues[dest] = node;
    } else {
        ipc_queue_node_t* curr = thread_queues[dest];
        while (curr->next) curr = curr->next;
        curr->next = node;
    }
    
    return true;
}

bool vxair_ipc_recv(vxair_msg_t* msg, bool block) {
    vxair_tid_t tid = vxair_sched_get_current_tid();
    if (tid >= 1024) return false;
    
    while (1) {
        if (thread_queues[tid]) {
            ipc_queue_node_t* node = thread_queues[tid];
            thread_queues[tid] = node->next;
            *msg = node->msg;
            vxair_kfree(node);
            return true;
        }
        
        if (!block) return false;
        
        vxair_sched_block();
    }
}
