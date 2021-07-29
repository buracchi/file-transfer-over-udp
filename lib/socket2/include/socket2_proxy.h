#pragma once

#include "socket2.h"

struct socket2_proxy {
	struct {
		struct socket2 socket2;
	} super;
	struct socket2* real_socket;
	struct socket2* input_socket;
	struct socket2* output_socket;
	struct socket2_proxy_vtbl* __ops_vptr;
};

static struct socket2_proxy_vtbl {
	int (*destroy)(struct socket2_proxy* socket2_proxy);
} __socket2_proxy_ops_vtbl = { 0 };

extern int socket2_proxy_init(struct socket2_proxy* proxy, int fd, struct tproto* tproto, struct nproto* nproto, struct socket2* socket2);

static inline int tproto_udp_destroy(struct socket2_proxy* proxy) {
	return proxy->__ops_vptr->destroy(proxy);
}
