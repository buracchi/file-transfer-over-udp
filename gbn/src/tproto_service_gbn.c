#include "tproto/tproto_service_gbn.h"

#include <stddef.h>
#include <stdint.h>

#include <buracchi/common/networking/types/tproto_service.h>
#include <buracchi/common/containers/map/linked_list_map.h>
#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/try.h>
#include <statemachine.h>

#include "gbn_statemachine.h"

struct cmn_tproto_service_gbn {
    struct cmn_tproto_service super;
    cmn_map_t state_machine_map;
};

static inline cmn_map_t get_state_machine_map();

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

static struct cmn_tproto_service_gbn service = {
        .super = {
                .__ops_vptr = &__cmn_tproto_service_ops_vtbl,
                .type = SOCK_DGRAM,
                .protocol = 0
        },
        .state_machine_map = NULL
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
extern struct cmn_tproto_service* cmn_tproto_service_gbn = &(service.super);
#elif __unix__
struct cmn_tproto_service *cmn_tproto_service_gbn = &(service.super);
#endif

static inline cmn_map_t get_state_machine_map() {
    if (!service.state_machine_map) {
        service.state_machine_map = (cmn_map_t) cmn_linked_list_map_init();
    }
    return service.state_machine_map;
}

static int _accept(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t *addr_len) {
    gbn_statemachine_t statemachine;
    cmn_map_at(get_state_machine_map(), socket, (void **) &statemachine);
    // update state machine info
    cmn_statemachine_send_event((cmn_statemachine_t) statemachine, GBN_EV_ACCEPT);
    return -1;
}

static int _connect(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr, socklen_t addr_len) {
    gbn_statemachine_t statemachine;
    cmn_map_at(get_state_machine_map(), socket, (void **) &statemachine);
    gbn_statemachine_set_peer_addr(statemachine, addr, addr_len);
    cmn_statemachine_send_event((cmn_statemachine_t) statemachine, GBN_EV_CONNECT);
    return -1;
}

static int _listen(cmn_tproto_service_t service, cmn_socket2_t socket, int backlog) {
    return -1;
}

static ssize_t _peek(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n) {
    return -1;
}

static ssize_t _recv(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n) {
    gbn_statemachine_t statemachine;
    cmn_map_at(get_state_machine_map(), socket, (void **) &statemachine);
    // update state machine info
    cmn_statemachine_send_event((cmn_statemachine_t) statemachine, GBN_EV_RECV);
    return -1;
}

static ssize_t _send(cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t *buff, uint64_t n) {
    gbn_statemachine_t statemachine;
    cmn_map_at(get_state_machine_map(), socket, (void **) &statemachine);
    // update state machine info
    cmn_statemachine_send_event((cmn_statemachine_t) statemachine, GBN_EV_SEND);
    return -1;
}

static int _close(cmn_tproto_service_t service, cmn_socket2_t socket) {
    gbn_statemachine_t statemachine;
    cmn_map_at(get_state_machine_map(), socket, (void **) &statemachine);
    // update state machine info
    cmn_statemachine_send_event((cmn_statemachine_t) statemachine, GBN_EV_CLOSE);
    return -1;
}
