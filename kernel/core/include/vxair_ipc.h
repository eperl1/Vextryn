#pragma once
#include "vxair_types.h"

/**
 * @file vxair_ipc.h
 * @brief Inter-Process Communication
 */

typedef struct vxair_msg {
    vxair_tid_t sender;
    uint64_t msg_type;
    uint64_t data[6];
} vxair_msg_t;

// Inter-Process Communication
void vxair_ipc_init(void);
bool vxair_ipc_send(vxair_tid_t dest, vxair_msg_t* msg);
bool vxair_ipc_recv(vxair_msg_t* msg, bool block);
