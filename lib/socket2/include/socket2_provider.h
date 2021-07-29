#pragma once

#include <string.h>
#include <sys/socket.h>

#include "socket2.h"
#include "socket2_proxy.h"
#include "tproto.h"
#include "nproto.h"
#include "nproto/nproto_unix.h"
#include "tproto/tproto_udp.h"
#include "try.h"

static inline int get_socket2(struct socket2* socket2, struct tproto* tproto, struct nproto* nproto) {
	struct nproto_unix unix;
	struct tproto_udp udp;
	struct socket2 s;
	struct socket2_proxy p;
	int fds[2];
	nproto_unix_init(&unix);
	tproto_udp_init(&udp);
	try(socket2_init(&s, tproto, nproto), 1, error);
	if (!tproto_is_kernel_handled(tproto)) {
		try(socketpair(&unix.super.nproto.domain, &udp.super.tproto.type, 0, &fds), -1, error);
		socket2_proxy_init(&p, fds[0], &udp.super.tproto, &unix.super.nproto, &s);
		memcpy(socket2, &p.super.socket2, sizeof * socket2);
	}
	else {
		memcpy(socket2, &s, sizeof * socket2);
	}
	return 0;
error:
	return 1;
}