#include "request_handler.h"

#include <string.h>

#include "struct_request_handler.h"

static struct request_handler_vtbl* get_request_handler_vtbl();

extern void request_handler_handle_request(request_handler_t request_handler, struct socket2* socket) {
	struct request_handler* this = (struct request_handler*)request_handler;
	_request_handler_ops(this)->handle_request(this, socket);
}

extern void _request_handler_init(struct request_handler* this) {
	this->__ops_vptr = get_request_handler_vtbl();
}

static struct request_handler_vtbl* get_request_handler_vtbl() {
	struct request_handler_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__request_handler_ops_vtbl, sizeof * &__request_handler_ops_vtbl)) {
		__request_handler_ops_vtbl.handle_request = NULL;
	}
	return &__request_handler_ops_vtbl;
}
