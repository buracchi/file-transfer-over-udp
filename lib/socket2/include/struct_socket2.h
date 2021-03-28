#pragma once

struct socket2 {
	int fd;
	struct sockaddr2* address;
	struct tproto* tproto;
	struct nproto* nproto;
};
