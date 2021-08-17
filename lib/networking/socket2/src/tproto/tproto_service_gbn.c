#include "tproto_service.h"
#include "tproto/tproto_service_gbn.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "socket2.h"
#include "try.h"
#include "utilities.h"

static ssize_t _peek(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
static ssize_t _recv(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
static ssize_t _send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n);

static struct cmn_tproto_service_vtbl __cmn_tproto_service_ops_vtbl = { 
    .peek = _peek,
    .recv = _recv,
    .send = _send
};

static struct cmn_tproto_service_gbn service = {
    .super = {
        .__ops_vptr = &__cmn_tproto_service_ops_vtbl,
        .type = SOCK_STREAM,
        .protocol = 0
    }
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
    extern struct cmn_tproto_service* cmn_tproto_service_gbn = &(service.super);
#elif __unix__
    struct cmn_tproto_service* cmn_tproto_service_gbn = &(service.super);
#endif

static ssize_t _peek(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return recv(socket->fd, buff, n, MSG_PEEK);
}

static ssize_t _recv(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return recv(socket->fd, buff, n, 0);
}

static ssize_t _send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n) {
    return send(socket->fd, buff, n, 0);
}
