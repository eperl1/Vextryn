#ifndef VXAIR_NET_SOCKET_H
#define VXAIR_NET_SOCKET_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Socket domains */
#define VXAIR_AF_INET 2

/* Socket types */
#define VXAIR_SOCK_STREAM 1
#define VXAIR_SOCK_DGRAM 2

/* IP protocols */
#define VXAIR_IPPROTO_TCP 6
#define VXAIR_IPPROTO_UDP 17

/**
 * @struct vxair_sockaddr_in
 * @brief Socket address, internet style.
 */
struct vxair_sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
};

/**
 * @brief Create an endpoint for communication.
 */
int vxair_socket(int domain, int type, int protocol);

/**
 * @brief Bind a name to a socket.
 */
int vxair_bind(int sockfd, const void *addr, size_t addrlen);

/**
 * @brief Listen for connections on a socket.
 */
int vxair_listen(int sockfd, int backlog);

/**
 * @brief Accept a connection on a socket.
 */
int vxair_accept(int sockfd, void *addr, size_t *addrlen);

/**
 * @brief Initiate a connection on a socket.
 */
int vxair_connect(int sockfd, const void *addr, size_t addrlen);

/**
 * @brief Send a message on a socket.
 */
ssize_t vxair_send(int sockfd, const void *buf, size_t len, int flags);

/**
 * @brief Receive a message from a socket.
 */
ssize_t vxair_recv(int sockfd, void *buf, size_t len, int flags);

/**
 * @brief Close a file descriptor.
 */
int vxair_close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_NET_SOCKET_H
