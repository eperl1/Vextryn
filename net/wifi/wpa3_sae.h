#ifndef VXAIR_NET_WIFI_WPA3_SAE_H
#define VXAIR_NET_WIFI_WPA3_SAE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file wpa3_sae.h
 * @brief WPA3 Simultaneous Authentication of Equals (SAE) protocol for Vextryn Air OS.
 */

#define VXAIR_SAE_MAX_PRIME_LEN 256
#define VXAIR_SAE_MAX_COMMIT_LEN 1024
#define VXAIR_SAE_CONFIRM_LEN 32
#define VXAIR_SAE_PMK_LEN 32

/**
 * @brief SAE Authentication states.
 */
typedef enum {
    VXAIR_SAE_STATE_NOT_STARTED = 0,
    VXAIR_SAE_STATE_COMMITTED,
    VXAIR_SAE_STATE_CONFIRMED,
    VXAIR_SAE_STATE_ACCEPTED
} vxair_sae_state_t;

/**
 * @brief SAE context for a given authentication session.
 */
typedef struct {
    vxair_sae_state_t state;
    uint8_t peer_mac[6];
    uint8_t local_mac[6];
    
    char password[64];
    size_t password_len;
    
    uint16_t group_id;
    uint16_t send_confirm_num;
    
    uint8_t pmk[VXAIR_SAE_PMK_LEN];
    uint8_t pmkid[16];
    
    uint8_t scalar_val[VXAIR_SAE_MAX_PRIME_LEN];
    uint8_t element_val[VXAIR_SAE_MAX_PRIME_LEN * 2 + 1];
    size_t scalar_len;
    size_t element_len;
    
    uint8_t kck[32];
} vxair_sae_context_t;

/**
 * @brief Initialize a new SAE session.
 * @param ctx Context to initialize.
 * @param local_mac Local MAC address.
 * @param peer_mac Peer (AP) MAC address.
 * @param password Network password/passphrase.
 * @return 0 on success.
 */
int vxair_sae_init(vxair_sae_context_t *ctx, const uint8_t *local_mac, const uint8_t *peer_mac, const char *password);

/**
 * @brief Generate an SAE Commit message to send to the peer.
 * @param ctx SAE context.
 * @param buffer Output buffer for commit message.
 * @param buf_len Length of output buffer, updated with actual length.
 * @return 0 on success, negative error code on failure.
 */
int vxair_sae_generate_commit(vxair_sae_context_t *ctx, uint8_t *buffer, size_t *buf_len);

/**
 * @brief Process an incoming SAE Commit message from the peer.
 * @param ctx SAE context.
 * @param data Received commit data.
 * @param len Length of received data.
 * @return 0 on success, negative error code on failure.
 */
int vxair_sae_process_commit(vxair_sae_context_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Generate an SAE Confirm message.
 * @param ctx SAE context.
 * @param buffer Output buffer.
 * @param buf_len Output buffer size, updated with actual length.
 * @return 0 on success, negative error code on failure.
 */
int vxair_sae_generate_confirm(vxair_sae_context_t *ctx, uint8_t *buffer, size_t *buf_len);

/**
 * @brief Process an incoming SAE Confirm message.
 * @param ctx SAE context.
 * @param data Received confirm data.
 * @param len Length of received data.
 * @return 0 on success, negative error code on failure.
 */
int vxair_sae_process_confirm(vxair_sae_context_t *ctx, const uint8_t *data, size_t len);

#endif /* VXAIR_NET_WIFI_WPA3_SAE_H */
