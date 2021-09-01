#pragma once

#include "gbn.h"

#include <stdint.h>
#include <event2/event.h>

struct receiver_state {
    int fd;
    int expectedseqnum;
    uint8_t* rslt_ptr;
    uint8_t* const rslt_end_ptr;
    struct event_base* event_base;
};

extern void on_receiver_readable(evutil_socket_t fd, short events, void* arg);
