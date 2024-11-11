#ifndef TFTP_CLIENT_SOCKET_H
#define TFTP_CLIENT_SOCKET_H

#include <netdb.h>
#include <sys/socket.h>

#include <logger.h>

struct server_sockaddr {
    struct sockaddr_storage sockaddr;
    socklen_t socklen;
};

struct tftp_client_connection {
    int sockfd;
    struct server_sockaddr server_addr;
    struct sockaddr_storage sock_addr_storage;
    socklen_t sock_addr_len;
    char sock_addr_str[INET6_ADDRSTRLEN];
    uint16_t sock_port;
};

bool tftp_client_connection_init(struct tftp_client_connection connection[static 1],
                                 const char host[static 1],
                                 const char service[static 1],
                                 struct logger logger[static 1]);


bool tftp_client_connection_set_recv_timeout(struct tftp_client_connection connection[static 1],
                                             uint8_t timeout_s,
                                             struct logger logger[static 1]);

bool tftp_client_connection_destroy(struct tftp_client_connection connection[static 1], struct logger logger[static 1]);

#endif // TFTP_CLIENT_SOCKET_H
