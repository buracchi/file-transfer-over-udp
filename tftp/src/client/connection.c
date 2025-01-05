#include "connection.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "../utils/inet.h"

static constexpr struct addrinfo hints_ipv4 = {
    .ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG,
    .ai_family = AF_INET,
    .ai_socktype = SOCK_DGRAM,
    .ai_addr = nullptr,
    .ai_canonname = nullptr,
    .ai_next = nullptr
};

static constexpr struct addrinfo hints_ipv6 = {
    .ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG,
    .ai_family = AF_INET6,
    .ai_socktype = SOCK_DGRAM,
    .ai_addr = nullptr,
    .ai_canonname = nullptr,
    .ai_next = nullptr
};

static inline bool is_ipv6_address(const char *address) {
    struct addrinfo *result = nullptr;
    int ret = getaddrinfo(address, nullptr, &hints_ipv6, &result);
    if (result != nullptr) {
        freeaddrinfo(result);
    }
    return ret == 0;
}

bool connection_init(struct connection connection[static 1],
                     const char host[static 1],
                     const char service[static 1],
                     struct logger logger[static 1]) {
    *connection = (struct connection) {
        .sockfd = -1,
        .sock_addr_len = sizeof connection->sock_addr_storage,
    };
    bool is_host_ipv6 = is_ipv6_address(host);
    const struct addrinfo* hints = is_host_ipv6 ? &hints_ipv6 : &hints_ipv4;
    struct addrinfo *addrinfo_list = nullptr;
    int gai_ret = getaddrinfo(host, service, hints, &addrinfo_list);
    if (gai_ret) {
        logger_log_error(logger, "Could not translate the host and service required to a valid internet address. %s", gai_strerror(gai_ret));
        if (addrinfo_list != nullptr) {
            freeaddrinfo(addrinfo_list);
        }
        return false;
    }
    struct addrinfo *addrinfo = addrinfo_list;
    connection->sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (connection->sockfd == -1) {
        logger_log_error(logger, "Could not create a socket. %s", strerror(errno));
        goto fail;
    }
    if (setsockopt(connection->sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {true}, sizeof(int)) == -1 ||
        (is_host_ipv6 &&
         setsockopt(connection->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {false}, sizeof(int)) == -1)) {
        logger_log_error(logger, "Could not set socket options. %s", strerror(errno));
        goto fail2;
    }
    memcpy(&connection->server_addr.sockaddr, addrinfo->ai_addr, addrinfo->ai_addrlen);
    connection->server_addr.socklen = addrinfo->ai_addrlen;
    freeaddrinfo(addrinfo_list);
    return true;
fail2:
    close(connection->sockfd);
fail:
    freeaddrinfo(addrinfo_list);
    return false;
}

bool connection_set_recv_timeout(struct connection connection[static 1], uint8_t timeout_s, struct logger logger[static 1]) {
    struct timeval timeout = {.tv_sec = timeout_s,};
    if (setsockopt(connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) == -1) {
        logger_log_error(logger, "Could not set socket timeout option. %s", strerror(errno));
        return false;
    }
    return true;
}

bool connection_destroy(struct connection connection[static 1], struct logger logger[static 1]) {
    if (close(connection->sockfd)) {
        logger_log_error(logger, "Could not close socket: %s", strerror(errno));
        return false;
    }
    return true;
}
