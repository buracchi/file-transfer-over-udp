#pragma once

#include "socket2.h"

struct cmn_request_handler {
    struct cmn_request_handler_vtbl *__ops_vptr;
};

static struct cmn_request_handler_vtbl {
    int (*destroy)(cmn_request_handler_t request_handler);

    void (*handle_request)(cmn_request_handler_t request_handler, cmn_socket2_t socket);
} __cmn_request_handler_ops_vtbl = {0, 0};
