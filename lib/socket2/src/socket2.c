#include "socket2.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "try.h"

#ifdef __unix__
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h> 
#include <errno.h>
#endif

#define INVALID_SOCKET -1
#define SOCKET_ERROR  -1

struct socket2 {
	int fd;
	struct sockaddr* addr;
	socklen_t addrlen;
	enum transport_protocol tproto;
	enum network_protocol nproto;
};

extern socket2_t socket2_init(enum transport_protocol tproto, enum network_protocol nproto) {
	struct socket2* socket2;
	socket2 = malloc(sizeof * socket2);
	if (socket2) {
		socket2->tproto = tproto;
		socket2->nproto = nproto;
		int type;
		switch (tproto) {
		case TCP:
			type = SOCK_STREAM;
			break;
		case UDP:
			type = SOCK_DGRAM;
			break;
		case RAW:
			type = SOCK_RAW;
			break;
		default:
			goto fail;
		}
		switch (nproto) {
		case IPV4: {
			struct sockaddr_in* paddr_in;
			try(socket2->fd = socket(AF_INET, type, 0), INVALID_SOCKET, fail);
			try(paddr_in = malloc(sizeof * paddr_in), NULL, fail2);
			memset(paddr_in, 0, sizeof * paddr_in);
			paddr_in->sin_family = AF_INET;
			socket2->addr = (struct sockaddr*)paddr_in;
			socket2->addrlen = sizeof * paddr_in;
			break;
		}
		case IPV6:
			break;
#ifdef __unix__
		case UNIX: {
			struct sockaddr_un* paddr_un;
			try(socket2->fd = socket(AF_UNIX, type, 0), INVALID_SOCKET, fail);
			try(paddr_un = malloc(sizeof * paddr_un), NULL, fail2);
			memset(paddr_un, 'x', sizeof * paddr_un);
			paddr_un->sun_family = AF_UNIX;
			paddr_un->sun_path[0] = '\0';
			socket2->addr = (struct sockaddr*)paddr_un;
			socket2->addrlen = 0;
			break;
		}
#endif
		default:
			goto fail;
		}
	}
	return socket2;
fail2:
	close(socket2->fd);
fail:
	free(socket2);
	return NULL;
}

extern void socket2_destroy(const socket2_t handle) {
	struct socket2* socket2 = handle;
	free(socket2->addr);
	free(socket2);
}

extern socket2_t socket2_accept(const socket2_t handle) {
	struct socket2* socket2 = handle;
	struct socket2* accepted;
	accepted = malloc(sizeof * accepted);
	if (accepted) {
		switch (socket2->nproto) {
		case IPV4:
			accepted->addrlen = sizeof(struct sockaddr_in);
			break;
		case IPV6:
			break;
#ifdef __unix__
		case UNIX:
			accepted->addrlen = sizeof(struct sockaddr_un);
			break;
#endif
		default:
			goto fail;
		}
		try(accepted->addr = malloc(accepted->addrlen), NULL, fail);
		while ((accepted->fd = accept(socket2->fd, accepted->addr, &accepted->addrlen)) == -1) {
			try(errno != EMFILE, true, fail2);
			sleep(0.1);
		}
	}
	return accepted;
fail2:
	free(accepted->addr);
fail:
	free(accepted);
	return NULL;
}

extern int socket2_connect(const socket2_t handle) {
	struct socket2* socket2 = handle;
	try(connect(socket2->fd, socket2->addr, socket2->addrlen), SOCKET_ERROR, fail);
	return 0;
fail:
	return -1;
}

extern int socket2_close(const socket2_t handle) {
	struct socket2* socket2 = handle;
	try(close(socket2->fd), -1, fail);
	return 0;
fail:
	return 1;
}

extern ssize_t socket2_recv(const socket2_t handle, char** buff) {
	struct socket2* socket2 = handle;
	size_t length = 2048;
	size_t b_recv;
	char* msg;
	try(msg = malloc(sizeof * msg * (length)), NULL, fail);
	memset(msg, 0, sizeof * msg * (length));
	try(b_recv = recv(socket2->fd, msg, length - 1, 0), -1, fail2);
	*buff = msg;
	return b_recv;
fail2:
	free(msg);
fail:
	return -1;
}

extern ssize_t socket2_send(const socket2_t handle, const char* buff) {
	struct socket2* socket2 = handle;
	int b_sent;
	try(b_sent = send(socket2->fd, buff, strlen(buff), 0), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern int socket2_listen(const socket2_t handle, int backlog) {
	struct socket2* socket2 = handle;
	try(bind(socket2->fd, socket2->addr, socket2->addrlen), -1, fail);
	try(listen(socket2->fd, backlog), -1, fail);
	return 0;
fail:
	return 1;
}

extern int socket2_get_fd(const socket2_t handle) {
	struct socket2* socket2 = handle;
	return socket2->fd;
}

extern int socket2_set_blocking(const socket2_t handle, bool blocking) {
	struct socket2* socket2 = handle;
	if (socket2->fd > 0) {
#ifdef _WIN32
		unsigned long mode = blocking ? 0 : 1;
		try(ioctlsocket(fd, FIONBIO, &mode), SOCKET_ERROR, fail);
		return 0;
#elif __unix__
		int flags;
		try (flags = fcntl(socket2->fd, F_GETFL, 0), -1, fail);
		flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
		try(fcntl(socket2->fd, F_SETFL, flags), !0, fail);
		return 0;
#endif
	}
fail:
	return 1;
}

extern int socket2_ipv4_setaddr(const socket2_t handle, const char* address, const uint16_t port) {
	struct socket2* socket2 = handle;
	struct sockaddr_in* paddr_in = (struct sockaddr_in*)socket2->addr;
	struct in_addr haddr;
	try(inet_aton(address, &haddr), 0, fail);
	paddr_in->sin_addr = haddr;
	paddr_in->sin_port = htons(port);
	return 0;
fail:
	return 1;
}

#ifdef __unix__
extern int socket2_unix_setaddr(const socket2_t handle, const char* address) {
	struct socket2* socket2 = handle;
	struct sockaddr_un* paddr_un = (struct sockaddr_un*)socket2->addr;
	strncpy(paddr_un->sun_path + 1, address, strlen(address));
	socket2->addrlen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(address));
	return 0;
}
#endif
