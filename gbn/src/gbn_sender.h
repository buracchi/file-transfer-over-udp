#pragma once

#include "gbn.h"

#include <stdint.h>
#include <event2/event.h>

struct sender_state {
    int fd;
    int base;
    int nextseqnum;
    const uint8_t* data_ptr;
    const uint8_t* const data_end_ptr;
    struct event_base* event_base;
    struct event* timeout_event;
    struct gbn_packet sndpkt[WINDOW_SIZE];
};

extern void on_sender_writeable(evutil_socket_t fd, short events, void* arg);
extern void on_sender_readable(evutil_socket_t fd, short events, void* arg);
extern void on_sender_timeout(evutil_socket_t fd, short events, void* arg);
