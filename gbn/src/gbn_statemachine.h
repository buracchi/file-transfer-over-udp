#pragma once

#include <buracchi/common/networking/socket2.h>

enum gbn_event {
    GBN_EV_ACCEPT,
    GBN_EV_CONNECT,
    GBN_EV_LISTEN,
    GBN_EV_PEEK,
    GBN_EV_RECV,
    GBN_EV_SEND,
    GBN_EV_CLOSE,
    GBN_EV_SOCKET_READABLE,
    GBN_EV_SOCKET_WRITABLE,
    GBN_EV_TIMEOUT,
};

typedef struct gbn_statemachine *gbn_statemachine_t;

extern gbn_statemachine_t gbn_statemachine_init(cmn_socket2_t socket);

extern void gbn_statemachine_set_peer_addr(gbn_statemachine_t statemachine, struct sockaddr *addr, socklen_t addr_len);
