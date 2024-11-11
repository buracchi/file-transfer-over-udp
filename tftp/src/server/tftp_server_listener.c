#include <buracchi/tftp/server_listener.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../utils/inet.h"
#include "../utils/utils.h"

constexpr size_t msg_control_size = 4096;

static const struct addrinfo hints_ipv4 = {
    .ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_ADDRCONFIG,
    .ai_family = AF_INET,
    .ai_socktype = SOCK_DGRAM,
    .ai_addr = nullptr,
    .ai_canonname = nullptr,
    .ai_next = nullptr
};

static const struct addrinfo hints_ipv6 = {
    .ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_ADDRCONFIG,
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

bool tftp_server_listener_init(struct tftp_server_listener listener[static 1],
                          const char host[static 1],
                          const char service[static 1],
                          struct logger logger[static 1]) {
    *listener = (struct tftp_server_listener) {
        .file_descriptor = -1,
        .msghdr = {
            .msg_iov = listener->iovec,
            .msg_iovlen = 1,
            .msg_control = malloc(msg_control_size),
            .msg_controllen = msg_control_size,
            .msg_flags = MSG_CTRUNC,
        },
    };
    if (listener->msghdr.msg_control == nullptr) {
        logger_log_error(logger, "Failed to initialize msghdr control buffer. %s", strerror_rbs(errno));
        return false;
    }
    bool is_host_ipv6 = is_ipv6_address(host);
    const struct addrinfo* hints = is_host_ipv6 ? &hints_ipv6 : &hints_ipv4;
    struct addrinfo *addrinfo_list = nullptr;
    int gai_ret = getaddrinfo(host, service, hints, &addrinfo_list);
    if (gai_ret) {
        const char *error_message = gai_strerror(gai_ret);
        logger_log_error(logger, "Could not translate the host and service required to a valid internet address. %s", error_message);
        if (addrinfo_list != nullptr) {
            freeaddrinfo(addrinfo_list);
        }
        return false;
    }
    for (struct addrinfo *addrinfo = addrinfo_list; addrinfo != nullptr; addrinfo = addrinfo->ai_next) {
        char bind_addr_str[INET6_ADDRSTRLEN] = {};
        uint16_t bind_addr_port;
        listener->file_descriptor = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if (listener->file_descriptor == -1) {
            logger_log_error(logger, "Could not create a socket for the server. %s", strerror_rbs(errno));
            goto fail;
        }
        if (setsockopt(listener->file_descriptor, SOL_SOCKET, SO_REUSEADDR, &(int) {true}, sizeof(int)) == -1 ||
            setsockopt(listener->file_descriptor, IPPROTO_IP, IP_RECVORIGDSTADDR, &(int) {true}, sizeof(int)) == -1 ||
            (is_host_ipv6 &&
            (setsockopt(listener->file_descriptor, IPPROTO_IPV6, IPV6_RECVORIGDSTADDR, &(int) {true}, sizeof(int)) == -1 ||
             setsockopt(listener->file_descriptor, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {false}, sizeof(int)) == -1))) {
            logger_log_error(logger, "Could not set listener socket options for the server. %s", strerror_rbs(errno));
            goto fail2;
        }
        if (inet_ntop_address(addrinfo->ai_addr, bind_addr_str, &bind_addr_port) == nullptr) {
            logger_log_error(logger, "Could not translate binding address to a string representation. %s", strerror_rbs(errno));
            goto fail2;
        }
        logger_log_debug(logger, "Trying to bind to host %s on port %hu", bind_addr_str, bind_addr_port);
        if (bind(listener->file_descriptor, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
            logger_log_debug(logger, "Can't bind to this address. %s", strerror_rbs(errno));
            close(listener->file_descriptor);
            continue;
        }
        logger_log_info(logger, "Server is listening on %s port %hu", bind_addr_str, bind_addr_port);
        memcpy(&listener->addr_storage, addrinfo->ai_addr, addrinfo->ai_addrlen);
        listener->addrinfo = (struct addrinfo) {
            .ai_addr = (struct sockaddr *) &listener->addr_storage,
            .ai_addrlen = addrinfo->ai_addrlen,
            .ai_family = addrinfo->ai_family,
            .ai_socktype = addrinfo->ai_socktype,
            .ai_flags = addrinfo->ai_flags,
            .ai_protocol = addrinfo->ai_protocol,
            .ai_next = nullptr,
            .ai_canonname = nullptr,
        };
        freeaddrinfo(addrinfo_list);
        return true;
    }
    logger_log_error(logger, "Failed to bind server to any address");
    if (listener->file_descriptor == -1) {
        goto fail;
    }
fail2:
    close(listener->file_descriptor);
fail:
    freeaddrinfo(addrinfo_list);
    return false;
}

void tftp_server_listener_set_recvmsg_dest(struct tftp_server_listener listener[static 1], struct tftp_peer_message dest[static 1]) {
    listener->msghdr.msg_controllen = msg_control_size;
    listener->msghdr.msg_flags = 0;
    memset(listener->msghdr.msg_control, 0, msg_control_size);
    *dest = (struct tftp_peer_message) {
        .peer_addrlen = sizeof dest->peer_addr,
    };
    listener->msghdr.msg_name = &dest->peer_addr;
    listener->msghdr.msg_namelen = dest->peer_addrlen;
    listener->msghdr.msg_iov[0].iov_base = dest->buffer;
    listener->msghdr.msg_iov[0].iov_len = sizeof dest->buffer;
}

void tftp_server_listener_destroy(struct tftp_server_listener listener[static 1]) {
    close(listener->file_descriptor);
    free(listener->msghdr.msg_control);
}
