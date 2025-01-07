#include "session_connection.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../utils/inet.h"

bool session_connection_init(struct session_connection connection[static 1],
                             struct addrinfo session_addr[static 1],
                             struct sockaddr_storage client_addr,
                             socklen_t client_addrlen,
                             bool is_ipv4,
                             struct logger logger[static 1]) {
    *connection = (struct session_connection) {
        .sockfd = -1,
        .address = (struct inet_address) {
            .str = nullptr,
            .addrlen = session_addr->ai_addrlen,
            .sockaddr = (struct sockaddr *) &connection->address.storage,
        },
        .client_address = (struct inet_address) {
            .str = nullptr,
            .storage = client_addr,
            .addrlen = client_addrlen,
            .sockaddr = (struct sockaddr *) &connection->client_address.storage,
        },
        .msghdr = {},
        .iovec = {},
        .recv_buffer = malloc(tftp_default_blksize),
        .recv_buffer_size = tftp_default_blksize,
    };
    {   // TODO: Remove block statement when io_uring_prep_recvfrom will be available.
        connection->msghdr.msg_iov = connection->iovec;
        connection->msghdr.msg_iovlen = 1;
        connection->msghdr.msg_control = nullptr;
        connection->msghdr.msg_controllen = 0;
        connection->msghdr.msg_flags = 0;
    }
    
    if (connection->recv_buffer == nullptr) {
        logger_log_error(logger, "Could not allocate memory for the receive buffer.");
        return false;
    }
    memcpy(&connection->address.storage, session_addr->ai_addr, session_addr->ai_addrlen);
    
    if (is_ipv4) {
        logger_log_debug(logger, "Peer request an IPV4 response, using an IPV4 connection to support an IPV6 unaware client.");
        struct sockaddr_storage ipv4_serv_addr = {};
        struct sockaddr_storage ipv4_clnt_addr = {};
        if (sockaddr_in6_to_in((const struct sockaddr_in6 *) &connection->address.storage, (struct sockaddr_in *) &ipv4_serv_addr) < 0) {
            logger_log_error(logger, "Could not translate server address to an IPV4 address.");
            return false;
        }
        memcpy(&connection->address.storage, &ipv4_serv_addr, sizeof ipv4_serv_addr);
        connection->address.addrlen = sizeof(struct sockaddr_in);
        connection->address.storage.ss_family = AF_INET;
        if (sockaddr_in6_to_in((const struct sockaddr_in6 *) &connection->client_address.storage, (struct sockaddr_in *) &ipv4_clnt_addr) < 0) {
            logger_log_error(logger, "Could not translate peer address to an IPV4 address.");
            return false;
        }
        memcpy(&connection->client_address.storage, &ipv4_clnt_addr, sizeof ipv4_clnt_addr);
        connection->client_address.addrlen = sizeof(struct sockaddr_in);
    }
    else {
        connection->client_address.addrlen = sizeof(struct sockaddr_in6);
    }
    connection->sockfd = socket(connection->address.storage.ss_family, SOCK_DGRAM, 0);
    if (connection->sockfd == -1) {
        logger_log_error(logger, "Could not create socket: %s", strerror(errno));
        return false;
    }
    // use an available ephemeral port for binding
    in_port_t *port = is_ipv4 ?
                      &((struct sockaddr_in *) &connection->address.storage)->sin_port :
                      &((struct sockaddr_in6 *) &connection->address.storage)->sin6_port;
    *port = 0;
    if (bind(connection->sockfd, connection->address.sockaddr, connection->address.addrlen) == -1) {
        logger_log_error(logger, "Could not bind socket: %s", strerror(errno));
        close(connection->sockfd);
        return false;
    }
    if (getsockname(connection->sockfd, connection->address.sockaddr, &connection->address.addrlen) == -1) {
        logger_log_error(logger, "Could not get socket name: %s", strerror(errno));
        close(connection->sockfd);
        return false;
    }
    // set misc fields
    connection->last_message_address = (struct inet_address) {
        .str = nullptr,
        .storage = connection->client_address.storage,
        .addrlen = connection->client_address.addrlen,
        .sockaddr = (struct sockaddr *) &connection->last_message_address.storage,
    };
    if (sockaddr_ntop(connection->address.sockaddr, connection->address.str_storage, &connection->address.port) != nullptr) {
        connection->address.str = connection->address.str_storage;
    }
    else {
        logger_log_error(logger, "Could not translate session address to a string representation.");
    }
    if (sockaddr_ntop(connection->client_address.sockaddr, connection->client_address.str_storage, &connection->client_address.port) != nullptr) {
        connection->client_address.str = connection->client_address.str_storage;
        connection->last_message_address.str = connection->client_address.str;
        connection->last_message_address.port = connection->client_address.port;
    }
    else {
        logger_log_error(logger, "Could not translate client address to a string representation.");
    }
    return true;
}

bool session_connection_destroy(struct session_connection connection[static 1], struct logger logger[static 1]) {
    logger_log_debug(logger, "Closing connection socket.");
    free(connection->recv_buffer);
    return close(connection->sockfd);
}
