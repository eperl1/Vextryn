#include "vxair_input.h"
#include <stddef.h>
#include <string.h>

#define EVENT_QUEUE_SIZE 256

static vxair_input_event_t g_event_queue[EVENT_QUEUE_SIZE];
static volatile size_t g_queue_head = 0;
static volatile size_t g_queue_tail = 0;

void vxair_input_init(void) {
    g_queue_head = 0;
    g_queue_tail = 0;
    // In a real OS, this would initialize hardware interrupts for PS/2 or USB
}

bool vxair_input_poll_event(vxair_input_event_t* event) {
    if (g_queue_head == g_queue_tail) {
        return false; // Queue empty
    }
    
    if (event) {
        *event = g_event_queue[g_queue_head];
    }
    
    g_queue_head = (g_queue_head + 1) % EVENT_QUEUE_SIZE;
    return true;
}

void vxair_input_push_event(const vxair_input_event_t* event) {
    if (!event) return;
    
    size_t next_tail = (g_queue_tail + 1) % EVENT_QUEUE_SIZE;
    if (next_tail == g_queue_head) {
        // Queue full, drop event
        return;
    }
    
    g_event_queue[g_queue_tail] = *event;
    g_queue_tail = next_tail;
}
