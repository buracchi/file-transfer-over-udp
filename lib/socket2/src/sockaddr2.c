#include "sockaddr2.h"

#include <stdlib.h>

#include "try.h"

static struct sockaddr2_vtbl* get_sockaddr2_vtbl();
static int _destroy(struct sockaddr2*);

extern int sockaddr2_init(struct sockaddr2* this, struct sockaddr* addr, socklen_t addrlen) {
	try(this->addr = malloc(addrlen), NULL, fail);
	memcpy(this->addr, addr, addrlen);
	this->addrlen = addrlen;
	this->__ops_vptr = get_sockaddr2_vtbl();
	return 0;
fail:
	return 1;
}

static struct sockaddr2_vtbl* get_sockaddr2_vtbl() {
	struct sockaddr2_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__sockaddr2_ops_vtbl, sizeof * &__sockaddr2_ops_vtbl)) {
		__sockaddr2_ops_vtbl.destroy = _destroy;
	}
	return &__sockaddr2_ops_vtbl;
}

static int _destroy(struct sockaddr2* this) {
	free(this->addr);
	return 0;
}
