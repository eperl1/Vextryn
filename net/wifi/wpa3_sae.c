#include "wpa3_sae.h"
#include <string.h>

/**
 * @file wpa3_sae.c
 * @brief WPA3 Simultaneous Authentication of Equals (SAE) protocol implementation.
 */

/* Helper function to simulate cryptographic hash (HMAC/SHA256) */
static void vxair_crypto_hash(const uint8_t *data, size_t len, uint8_t *out) {
    /* Simple XOR hash for simulation purposes */
    for (int i = 0; i < 32; i++) {
        out[i] = 0;
    }
    for (size_t i = 0; i < len; i++) {
        out[i % 32] ^= data[i];
    }
}

int vxair_sae_init(vxair_sae_context_t *ctx, const uint8_t *local_mac, const uint8_t *peer_mac, const char *password) {
    if (!ctx || !local_mac || !peer_mac || !password) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(vxair_sae_context_t));
    ctx->state = VXAIR_SAE_STATE_NOT_STARTED;
    
    memcpy(ctx->local_mac, local_mac, 6);
    memcpy(ctx->peer_mac, peer_mac, 6);
    
    ctx->password_len = strlen(password);
    if (ctx->password_len >= sizeof(ctx->password)) {
        return -2;
    }
    strcpy(ctx->password, password);
    
    /* Typically group 19 (NIST P-256) */
    ctx->group_id = 19;
    ctx->send_confirm_num = 1;
    
    return 0;
}

int vxair_sae_generate_commit(vxair_sae_context_t *ctx, uint8_t *buffer, size_t *buf_len) {
    if (!ctx || !buffer || !buf_len) return -1;
    
    if (*buf_len < 2 + VXAIR_SAE_MAX_PRIME_LEN + VXAIR_SAE_MAX_PRIME_LEN) {
        return -2;
    }
    
    /* 1. Derive PWE (Password Authenticated Element) using Hunt-and-Peck */
    /* 2. Generate random scalar and element (simulated here) */
    ctx->scalar_len = 32; /* P-256 scalar length */
    ctx->element_len = 64; /* P-256 uncompressed point length (X and Y) */
    
    memset(ctx->scalar_val, 0xAA, ctx->scalar_len);
    memset(ctx->element_val, 0xBB, ctx->element_len);
    
    /* Build commit message */
    size_t offset = 0;
    buffer[offset++] = (ctx->group_id >> 0) & 0xFF;
    buffer[offset++] = (ctx->group_id >> 8) & 0xFF;
    
    memcpy(buffer + offset, ctx->scalar_val, ctx->scalar_len);
    offset += ctx->scalar_len;
    
    memcpy(buffer + offset, ctx->element_val, ctx->element_len);
    offset += ctx->element_len;
    
    *buf_len = offset;
    
    /* We don't transition state until we receive peer's commit, or we can say we are in committing state */
    return 0;
}

int vxair_sae_process_commit(vxair_sae_context_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || len < 2) return -1;
    
    uint16_t peer_group = data[0] | (data[1] << 8);
    if (peer_group != ctx->group_id) {
        return -2; /* Unsupported group */
    }
    
    /* Derive KCK and PMK */
    /* KCK is used for Confirm message HMAC */
    vxair_crypto_hash((const uint8_t *)ctx->password, ctx->password_len, ctx->kck);
    vxair_crypto_hash(ctx->kck, 32, ctx->pmk);
    
    ctx->state = VXAIR_SAE_STATE_COMMITTED;
    return 0;
}

int vxair_sae_generate_confirm(vxair_sae_context_t *ctx, uint8_t *buffer, size_t *buf_len) {
    if (!ctx || !buffer || !buf_len) return -1;
    
    if (ctx->state != VXAIR_SAE_STATE_COMMITTED) {
        return -2;
    }
    if (*buf_len < 2 + VXAIR_SAE_CONFIRM_LEN) {
        return -3;
    }
    
    buffer[0] = (ctx->send_confirm_num >> 0) & 0xFF;
    buffer[1] = (ctx->send_confirm_num >> 8) & 0xFF;
    
    /* Generate Confirm HMAC over Commit data and KCK */
    uint8_t hash_out[32];
    vxair_crypto_hash(ctx->kck, 32, hash_out);
    
    memcpy(buffer + 2, hash_out, VXAIR_SAE_CONFIRM_LEN);
    *buf_len = 2 + VXAIR_SAE_CONFIRM_LEN;
    
    ctx->send_confirm_num++;
    return 0;
}

int vxair_sae_process_confirm(vxair_sae_context_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data) return -1;
    
    if (ctx->state != VXAIR_SAE_STATE_COMMITTED) {
        return -2;
    }
    if (len < 2 + VXAIR_SAE_CONFIRM_LEN) {
        return -3;
    }
    
    /* Validate peer's Confirm HMAC using KCK */
    /* (Simulation: assume valid) */
    
    ctx->state = VXAIR_SAE_STATE_ACCEPTED;
    return 0;
}
