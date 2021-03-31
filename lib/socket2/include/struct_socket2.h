#pragma once

struct socket2 {
	int fd;
	struct sockaddr2* address;
	struct tproto* tproto;
	struct nproto* nproto;
	struct socket2_vtbl* __ops_vptr;
};

static struct socket2_vtbl {
	int (*destroy)(struct socket2*);
	struct socket2* (*accept)(struct socket2*);
	int (*connect)(struct socket2*, const char*);
	int (*close)(struct socket2*);
	int (*listen)(struct socket2*, const char*, int);
	ssize_t(*peek)(struct socket2*, uint8_t*, uint64_t);
	ssize_t(*recv)(struct socket2*, uint8_t*, uint64_t);
	ssize_t(*send)(struct socket2*, const uint8_t*, uint64_t);
} __socket2_ops = { 0 };
