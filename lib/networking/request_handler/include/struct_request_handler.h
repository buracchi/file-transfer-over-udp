#pragma once

#include "socket2.h"

#define _request_handler_ops(p) ((struct request_handler_vtbl*)(p->__ops_vptr))

/*******************************************************************************
*                                 Object members                               *
*******************************************************************************/

struct request_handler {
	void* __ops_vptr;
};

static struct request_handler_vtbl {
	void (*handle_request)(struct request_handler* this, struct socket2* socket);
} __request_handler_ops_vtbl = { 0 };

/*******************************************************************************
*                                 Constructors                                 *
*******************************************************************************/

extern void _request_handler_init(struct request_handler* this);
