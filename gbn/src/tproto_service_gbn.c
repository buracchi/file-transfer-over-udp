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

#include "types/tproto_service.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

#define WINDOW_SIZE 5
#define TIMEOUT 5000
#define PKT_LEN 1450

struct cmn_tproto_service_gbn {
    struct cmn_tproto_service super;
};

enum gbn_packet_type {
    PACKET,
    ACK
};

struct gbn_packet {
    enum gbn_packet_type type;
    size_t seqnum;
    uint16_t length;
    uint8_t* data;
};

struct sender_info {
    int fd;
    int base;
    int nextseqnum;
    const uint8_t* data_ptr;
    struct event* timeout_event;
    struct gbn_packet sndpkt[WINDOW_SIZE];
};

struct receiver_info {
    int fd;
    int expectedseqnum;
    uint8_t* rslt_ptr;
};

static ssize_t _peek(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
static ssize_t _recv(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
static ssize_t _send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n);

static void on_sender_writeable(evutil_socket_t fd, short events, void* arg);
static void on_sender_readable(evutil_socket_t fd, short events, void* arg);
static void on_sender_timeout(evutil_socket_t fd, short events, void* arg);
static void on_receiver_readable(evutil_socket_t fd, short events, void* arg);
static void make_pkt(struct gbn_packet* pkt, int seqnum, const uint8_t* data, enum gbn_packet_type type);

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
    struct receiver_info info = {
        .fd = cmn_socket2_get_fd(socket),
        .expectedseqnum = 1,
        .rslt_ptr = buff
    };
    struct event* readable_event;
    struct event_base* event_base;
	try(event_base = event_base_new(), NULL, fail);
	try(readable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_READ | EV_PERSIST, on_receiver_readable, &info), NULL, fail);
	try(event_add(readable_event, NULL), -1, fail);
	try(event_base_dispatch(event_base), -1, fail);
	event_free(readable_event);
	event_base_free(event_base);
    return (info.rslt_ptr - buff);
fail:
    return -1;
}

static ssize_t _send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n) {
    // return send(cmn_socket2_get_fd(socket), buff, n, 0);
    struct sender_info info = {
        .fd = cmn_socket2_get_fd(socket),
        .base = 1,
        .nextseqnum = 1,
        .data_ptr = buff
    };
    struct event* writeable_event;
	struct event* readable_event;
    struct event* timeout_event;
    struct event_base* event_base;
	try(event_base = event_base_new(), NULL, fail);
	try(writeable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_WRITE | EV_PERSIST, on_sender_writeable, &info), NULL, fail);
	try(readable_event = event_new(event_base, cmn_socket2_get_fd(socket), EV_READ | EV_PERSIST, on_sender_readable, &info), NULL, fail);
    try(timeout_event = evtimer_new(event_base, on_sender_timeout, &info), NULL, fail);
	try(event_add(writeable_event, NULL), -1, fail);
	try(event_add(readable_event, NULL), -1, fail);
    info.timeout_event = timeout_event;
	try(event_base_dispatch(event_base), -1, fail);
	event_free(writeable_event);
	event_free(readable_event);
	event_free(timeout_event);
	event_base_free(event_base);
    return (info.data_ptr - buff);
fail:
    return -1;
}

static void on_sender_writeable(evutil_socket_t fd, short events, void* arg) {
    struct sender_info* info = arg;
    if (info->nextseqnum < info->base + WINDOW_SIZE) {
        make_pkt(&(info->sndpkt[info->nextseqnum]), info->nextseqnum, info->data_ptr, PACKET);
        send(info->fd, &(info->sndpkt[info->nextseqnum]), PKT_LEN, 0);
        if (info->base == info->nextseqnum) {
            try(evtimer_add(info->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
        }
        info->nextseqnum++;
    }
fail:
    return;
}

static void on_sender_readable(evutil_socket_t fd, short events, void* arg) {
    struct sender_info* info = arg;
    struct gbn_packet rcvpkt;
    recv(info->fd, &rcvpkt, PKT_LEN, 0);
    if (rcvpkt.type == ACK && rcvpkt.seqnum + 1 == info->nextseqnum) {
        try(evtimer_del(info->timeout_event), -1, fail);
    }
    else {
        try(evtimer_add(info->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
    }
fail:
    return;
}

static void on_sender_timeout(evutil_socket_t fd, short events, void* arg) {
    struct sender_info* info = arg;
    try(evtimer_add(info->timeout_event, &((struct timeval){.tv_sec=0, .tv_usec=TIMEOUT})), -1, fail);
    for (int i = info->base; i < info->nextseqnum; i++) {
        send(info->fd, &(info->sndpkt[i]), PKT_LEN, 0);
    }
fail:
    return;
}

static void on_receiver_readable(evutil_socket_t fd, short events, void* arg) {
    struct receiver_info* info = arg;
    struct gbn_packet rcvpkt;
    struct gbn_packet sndpkt;
    recv(info->fd, &rcvpkt, PKT_LEN, 0);
    if (rcvpkt.seqnum == info->expectedseqnum) {
        for (int i = 0; i < rcvpkt.length; i++) {
            *(info->rslt_ptr++) = rcvpkt.data[i];
        }
        make_pkt(&sndpkt, info->expectedseqnum, NULL, ACK);
        send(info->fd, &sndpkt, PKT_LEN, 0);
        info->expectedseqnum++;
    }
    else {
        make_pkt(&sndpkt, info->expectedseqnum - 1, NULL, ACK);
        send(info->fd, &sndpkt, PKT_LEN, 0);
    }
}

static void make_pkt(struct gbn_packet* pkt, int seqnum, const uint8_t* data, enum gbn_packet_type type) {
    pkt->seqnum = seqnum;
    if (data) {
        // data must be copied
    }
    pkt->type = type;
}
