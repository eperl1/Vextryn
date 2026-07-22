#include "tcp.h"
#include <string.h>

#define VXAIR_MAX_TCP_CONNECTIONS 16

typedef enum {
    VXAIR_TCP_STATE_CLOSED,
    VXAIR_TCP_STATE_LISTEN,
    VXAIR_TCP_STATE_SYN_SENT,
    VXAIR_TCP_STATE_SYN_RECEIVED,
    VXAIR_TCP_STATE_ESTABLISHED,
    VXAIR_TCP_STATE_FIN_WAIT_1,
    VXAIR_TCP_STATE_FIN_WAIT_2,
    VXAIR_TCP_STATE_CLOSE_WAIT,
    VXAIR_TCP_STATE_CLOSING,
    VXAIR_TCP_STATE_LAST_ACK,
    VXAIR_TCP_STATE_TIME_WAIT
} vxair_tcp_state_t;

typedef struct {
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t remote_ip;
    vxair_tcp_state_t state;
    uint32_t snd_una;
    uint32_t snd_nxt;
    uint32_t rcv_nxt;
} vxair_tcp_connection_t;

static vxair_tcp_connection_t vxair_tcp_connections[VXAIR_MAX_TCP_CONNECTIONS];

/**
 * @brief Initialize TCP state machines and timers.
 */
void vxair_tcp_init(void) {
    for (int i = 0; i < VXAIR_MAX_TCP_CONNECTIONS; ++i) {
        vxair_tcp_connections[i].state = VXAIR_TCP_STATE_CLOSED;
    }
}

/**
 * @brief Process incoming TCP segments, manage state transitions.
 * @param packet Pointer to the TCP segment.
 * @param len Length of the segment.
 */
void vxair_tcp_receive(void *packet, uint16_t len) {
    if (len < sizeof(vxair_tcp_header_t)) {
        return;
    }
    
    /* vxair_tcp_header_t *tcp = (vxair_tcp_header_t *)packet; */
    
    /* Demultiplex to connection based on ports and IP */
    /* Handle state machine transitions (SYN, ACK, FIN, RST, etc.) */
}

/**
 * @brief Segment data and transmit TCP packets.
 * @param connection Connection state structure.
 * @param data Data to send.
 * @param len Length of data.
 * @return Bytes sent, -1 on error.
 */
int vxair_tcp_send(void *connection, const void *data, uint16_t len) {
    if (!connection || !data) return -1;
    
    vxair_tcp_connection_t *conn = (vxair_tcp_connection_t *)connection;
    if (conn->state != VXAIR_TCP_STATE_ESTABLISHED) {
        return -1;
    }
    
    /* Construct TCP segment, handle windowing and retransmission queues */
    return len;
}
