#pragma once

#include "struct_request_handler.h"

#include "ft_service.h"

#define _ft_handler_ops(p) ((struct ft_handler_vtbl*)(p->__ops_vptr))

/*******************************************************************************
*                                 Object members                               *
*******************************************************************************/

struct ft_handler {
	struct request_handler;
	const ft_service_t ft_service;
};

static struct ft_handler_vtbl {
	struct request_handler_vtbl;
	int (*destroy)(struct ft_handler* this);
} __ft_handler_ops_vtbl = { 0 };

/*******************************************************************************
*                                 Constructors                                 *
*******************************************************************************/

extern void _ft_handler_init(struct ft_handler* this, const ft_service_t ft_service);
