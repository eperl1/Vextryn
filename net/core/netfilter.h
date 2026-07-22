#ifndef VXAIR_NET_NETFILTER_H
#define VXAIR_NET_NETFILTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum vxair_nf_action_t
 * @brief Netfilter hook actions.
 */
typedef enum {
    VXAIR_NF_ACCEPT,
    VXAIR_NF_DROP,
    VXAIR_NF_QUEUE
} vxair_nf_action_t;

/**
 * @struct vxair_nf_rule_t
 * @brief Structure for a basic Netfilter rule.
 */
typedef struct {
    uint32_t src_ip;
    uint32_t src_mask;
    uint32_t dst_ip;
    uint32_t dst_mask;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    vxair_nf_action_t action;
} vxair_nf_rule_t;

/**
 * @brief Initialize default netfilter rules.
 */
void vxair_netfilter_init(void);

/**
 * @brief Evaluate packet against filter rules.
 * @param packet Pointer to the packet.
 * @param len Packet length.
 * @return The action to take (ACCEPT, DROP, QUEUE).
 */
vxair_nf_action_t vxair_netfilter_hook(void *packet, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_NETFILTER_H
