#include "tproto_service_gbn.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <event2/event.h>

#include "gbn_sender.h"
#include "gbn_receiver.h"
#include "types/tproto_service.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

struct cmn_tproto_service_gbn {
    struct cmn_tproto_service super;
};

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
        .type = SOCK_DGRAM,
        .protocol = 0
    }
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
    extern struct cmn_tproto_service* cmn_tproto_service_gbn = &(service.super);
#elif __unix__
    struct cmn_tproto_service* cmn_tproto_service_gbn = &(service.super);
#endif

static ssize_t _peek(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return recv(cmn_socket2_get_fd(socket), buff, n, MSG_PEEK);
}

static ssize_t _recv(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    struct receiver_state state = {
        .fd = cmn_socket2_get_fd(socket),
        .expectedseqnum = 1,
        .rslt_ptr = buff,
        .rslt_end_ptr = buff + n
    };
    struct event* readable_event;
    struct event_base* event_base;
	try(event_base = event_base_new(), NULL, fail);
	try(readable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_READ | EV_PERSIST, on_receiver_readable, &state), NULL, fail);
	try(event_add(readable_event, NULL), -1, fail);
    state.event_base = event_base;
	try(event_base_dispatch(event_base), -1, fail);
	event_free(readable_event);
	event_base_free(event_base);
    return (state.rslt_ptr - buff);
fail:
    return -1;
}

static ssize_t _send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n) {
    struct sender_state state = {
        .fd = cmn_socket2_get_fd(socket),
        .base = 1,
        .nextseqnum = 1,
        .data_ptr = buff,
        .data_end_ptr = buff + n
    };
    struct event* writeable_event;
	struct event* readable_event;
    struct event* timeout_event;
    struct event_base* event_base;
	try(event_base = event_base_new(), NULL, fail);
	try(writeable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_WRITE | EV_PERSIST, on_sender_writeable, &state), NULL, fail);
	try(readable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_READ | EV_PERSIST, on_sender_readable, &state), NULL, fail);
    try(timeout_event = evtimer_new(event_base, on_sender_timeout, &state), NULL, fail);
	try(event_add(writeable_event, NULL), -1, fail);
	try(event_add(readable_event, NULL), -1, fail);
    state.event_base = event_base;
    state.timeout_event = timeout_event;
	try(event_base_dispatch(event_base), -1, fail);
	event_free(writeable_event);
	event_free(readable_event);
	event_free(timeout_event);
	event_base_free(event_base);
    return (state.data_ptr - buff);
fail:
    return -1;
}