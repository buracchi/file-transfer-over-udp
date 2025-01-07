#ifndef TFTP_SERVER_CONNECTION_H
#define TFTP_SERVER_CONNECTION_H

#include <netdb.h>
#include <sys/socket.h>

#include <logger.h>
#include <tftp.h>

struct inet_address {
    char str_storage[INET6_ADDRSTRLEN];
    uint16_t port;
    struct sockaddr_storage storage;
    socklen_t addrlen;
    struct sockaddr *sockaddr; // type-casted pointer to storage
    char *str; // nullable pointer to string representation
};

struct session_connection {
    int sockfd;
    
    struct inet_address address;
    struct inet_address client_address;
    struct inet_address last_message_address;
    
    uint8_t *recv_buffer;
    size_t recv_buffer_size;
    // TODO: As of Linux kernel version 6.10.4 IO_URING_OP_RECVFROM is not implemented, this is a workaround.
    //  Remove this fields and use io_uring_prep_recvfrom when available.
    struct msghdr msghdr;
    struct iovec iovec[1];
};

bool session_connection_init(struct session_connection connection[static 1],
                             struct addrinfo session_addr[static 1],
                             struct sockaddr_storage client_addr,
                             socklen_t client_addrlen,
                             bool is_ipv4,
                             struct logger logger[static 1]);

bool session_connection_destroy(struct session_connection connection[static 1], struct logger logger[static 1]);

#endif // TFTP_SERVER_CONNECTION_H
