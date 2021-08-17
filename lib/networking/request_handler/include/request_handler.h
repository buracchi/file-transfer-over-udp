#pragma once

#include "socket2.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_request_handler {
	struct request_handler_vtbl* __ops_vptr;
};

static struct request_handler_vtbl {
	void (*handle_request)(struct cmn_request_handler* this, struct socket2* socket);
} __request_handler_ops_vtbl = { 0 };

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

static inline void cmn_request_handler_handle_request(struct cmn_request_handler* request_handler, struct socket2* socket) {
    request_handler->__ops_vptr->handle_request(request_handler, socket);
}
