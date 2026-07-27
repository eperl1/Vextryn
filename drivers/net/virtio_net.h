#ifndef VXAIR_VIRTIO_NET_H
#define VXAIR_VIRTIO_NET_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTQ_DESC_F_NEXT         1
#define VIRTQ_DESC_F_WRITE        2

#define VIRTIO_NET_FRAME_SIZE     2048
#define VIRTIO_NET_QUEUE_SIZE     64

// Mandatory virtio-net header (12 bytes, always required before every packet)
typedef struct __attribute__((packed)) {
    uint8_t  flags;          // 0 = no checksum offload
    uint8_t  gso_type;       // 0 = no GSO
    uint16_t hdr_len;        // 0 = not used
    uint16_t gso_size;       // 0 = not used
    uint16_t csum_start;     // 0 = not used
    uint16_t csum_offset;    // 0 = not used
    uint16_t num_buffers;    // 0 or 1 for simple mode
} vxair_virtio_net_hdr_t;

#define VIRTIO_NET_HDR_SIZE sizeof(vxair_virtio_net_hdr_t)  // 12 bytes

// Virtqueue descriptor
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} vxair_virtq_desc_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_NET_QUEUE_SIZE];
} vxair_virtq_avail_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    struct __attribute__((packed)) {
        uint32_t id;
        uint32_t len;
    } ring[VIRTIO_NET_QUEUE_SIZE];
} vxair_virtq_used_t;

// Legacy vring with page-aligned used ring
typedef struct __attribute__((aligned(4096))) {
    vxair_virtq_desc_t  desc[VIRTIO_NET_QUEUE_SIZE];
    vxair_virtq_avail_t avail;
    uint8_t             pad[2940];
    vxair_virtq_used_t  used;
} vxair_virtq_t;

typedef struct {
    bool found;
    uint16_t io_base;
    uint8_t mac_addr[6];
    vxair_virtq_t *tx_vq;
    vxair_virtq_t *rx_vq;
    uint16_t rx_next_used;
    uint16_t rx_avail_repost;

    // TX buffer with space for the mandatory 12-byte virtio-net header
    uint8_t tx_frame[VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE] __attribute__((aligned(16)));

    // Per-descriptor RX buffers (each includes space for the header)
    uint8_t rx_bufs[VIRTIO_NET_QUEUE_SIZE][VIRTIO_NET_HDR_SIZE + VIRTIO_NET_FRAME_SIZE] __attribute__((aligned(16)));
} vxair_virtio_net_t;

extern vxair_virtio_net_t g_virtio_net;

int vxair_virtio_net_init(void);
int vxair_virtio_net_send(const uint8_t *data, uint16_t len);
uint16_t vxair_virtio_net_receive(uint8_t *out_buf, uint16_t max_len);

#ifdef __cplusplus
}
#endif

#endif
