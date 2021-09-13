#include "tproto_service.h"

#include <unistd.h>

#include "types/tproto/tproto_service_tcp.h"
#include "types/socket2.h"

static int _accept(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t *addr_len);

static int _connect(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t addr_len);

static int _listen(cmn_tproto_service_t service, cmn_socket2_t socket, int backlog);

static ssize_t _peek(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n);

static ssize_t _recv(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n);

static ssize_t _send(cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t *buff, uint64_t n);

static int _close(cmn_tproto_service_t service, cmn_socket2_t socket);

static struct cmn_tproto_service_vtbl __cmn_tproto_service_ops_vtbl = {
        .accept = _accept,
        .connect = _connect,
        .listen = _listen,
        .peek = _peek,
        .recv = _recv,
        .send = _send,
        .close = _close
};

static struct cmn_tproto_service_tcp service = {
        .super = {
                .__ops_vptr = &__cmn_tproto_service_ops_vtbl,
                .type = SOCK_STREAM,
                .protocol = 0
        }
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
extern cmn_tproto_service_t cmn_tproto_service_tcp = &(service.super);
#elif __unix__
cmn_tproto_service_t cmn_tproto_service_tcp = &(service.super);
#endif

static int _accept(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t *addr_len) {
    return accept(socket->fd, addr, addr_len);
}

static int _connect(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t addr_len) {
    return connect(socket->fd, addr, addr_len);
}

static int _listen(cmn_tproto_service_t service, cmn_socket2_t socket, int backlog) {
    return listen(socket->fd, backlog);
}

static ssize_t _peek(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n) {
    return recv(socket->fd, buff, n, MSG_PEEK);
}

static ssize_t _recv(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n) {
    return recv(socket->fd, buff, n, 0);
}

static ssize_t _send(cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t *buff, uint64_t n) {
    return send(socket->fd, buff, n, 0);
}

static int _close(cmn_tproto_service_t service, cmn_socket2_t socket) {
    return close(socket->fd);
}
