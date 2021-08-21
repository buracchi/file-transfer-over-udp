#pragma once

#include "socket2.h"

struct cmn_request_handler {
	struct request_handler_vtbl* __ops_vptr;
};

static struct request_handler_vtbl {
	void (*handle_request)(cmn_request_handler_t request_handler, cmn_socket2_t socket);
} __cmn_request_handler_ops_vtbl = { 0 };
