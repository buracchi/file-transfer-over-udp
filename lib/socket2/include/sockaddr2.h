#pragma once

#include <sys/socket.h>

struct sockaddr2 {
    struct sockaddr* addr;
    socklen_t addrlen;
	struct sockaddr2_vtbl* __ops_vptr;
};

static struct sockaddr2_vtbl {
	int (*destroy)(struct sockaddr2* sockaddr2);
} __sockaddr2_ops_vtbl = { 0 };

extern int sockaddr2_init(struct sockaddr2* sockaddr2, struct sockaddr* addr, socklen_t addrlen);

static inline int sockaddr2_destroy(struct sockaddr2* sockaddr2) {
	sockaddr2->__ops_vptr->destroy(sockaddr2);
}
