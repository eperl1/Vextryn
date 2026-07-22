#ifndef VXAIR_NET_TLS_H
#define VXAIR_NET_TLS_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct vxair_tls_context_t
 * @brief TLS 1.3 minimal context.
 */
typedef struct {
    int socket_fd;
    int state;
    uint8_t session_key[32];
    uint8_t iv[12];
    uint64_t sequence_number;
} vxair_tls_context_t;

/**
 * @brief Initialize TLS subsystem.
 */
void vxair_tls_init(void);

/**
 * @brief Perform TLS 1.3 handshake.
 * @param context TLS context.
 * @return 0 on success, -1 on failure.
 */
int vxair_tls_handshake(void *context);

/**
 * @brief Encrypt and send data over TLS.
 * @param context TLS context.
 * @param data Data to send.
 * @param len Length of data.
 * @return Bytes sent, or -1 on error.
 */
ssize_t vxair_tls_encrypt_and_send(void *context, const void *data, size_t len);

/**
 * @brief Receive and decrypt data over TLS.
 * @param context TLS context.
 * @param buf Buffer for plaintext data.
 * @param len Length of buffer.
 * @return Bytes received, or -1 on error.
 */
ssize_t vxair_tls_receive_and_decrypt(void *context, void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_TLS_H
