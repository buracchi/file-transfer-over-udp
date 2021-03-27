#include "socket2.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "try.h"
#include "utilities.h"

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
#define CHUNK_SIZE 4096	// MUST be even

struct socket2 {
	int fd;
	struct sockaddr2* address;
	struct nproto* nproto;
	enum transport_protocol tproto;
};

extern socket2_t socket2_init(enum transport_protocol tproto, struct nproto* nproto) {
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
		try(socket2->fd = socket(nproto_get_domain(nproto), type, 0), INVALID_SOCKET, fail);
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
	free(socket2->address->addr);
	free(socket2->address);	// TODO: add destructor
	free(socket2);
}

extern socket2_t socket2_accept(const socket2_t handle) {
	struct socket2* socket2 = handle;
	struct socket2* accepted;
	accepted = malloc(sizeof * accepted);
	if (accepted) {
		accepted->nproto = socket2->nproto;
		accepted->address = malloc(sizeof * accepted->address);
		accepted->address->addr = malloc(socket2->address->addrlen);
		accepted->address->addrlen = socket2->address->addrlen;
		while ((accepted->fd = accept(socket2->fd, accepted->address->addr, &accepted->address->addrlen)) == -1) {
			try(errno != EMFILE, true, fail);
			sleep(0.1);
		}
	}
	return accepted;
fail:
	free(accepted);
	return NULL;
}

extern int socket2_connect(const socket2_t handle, const char* url) {
	struct socket2* socket2 = handle;
	try(socket2->address = nproto_get_sockaddr2(socket2->nproto, url), NULL, fail);
	try(connect(socket2->fd, socket2->address->addr, socket2->address->addrlen), SOCKET_ERROR, fail2);
	return 0;
fail2:
	free(socket2->address->addr);
	free(socket2->address);	// TODO: implement destructor
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

extern int socket2_listen(const socket2_t handle, const char* url, int backlog) {
	struct socket2* socket2 = handle;
	try(socket2->address = nproto_get_sockaddr2(socket2->nproto, url), NULL, fail);
	try(bind(socket2->fd, socket2->address->addr, socket2->address->addrlen), -1, fail2);
	try(listen(socket2->fd, backlog), -1, fail2);
	return 0;
fail2:
	free(socket2->address->addr);
	free(socket2->address);	// TODO: implement destructor
fail:
	return 1;
}

extern inline ssize_t socket2_peek(const socket2_t handle, uint8_t* buff, uint64_t n) {
	struct socket2* socket2 = handle;
	return recv(socket2->fd, buff, n, MSG_PEEK);
}

extern ssize_t socket2_recv(const socket2_t handle, uint8_t* buff, uint64_t n) {
	struct socket2* socket2 = handle;
	ssize_t b_recvd;
	try(b_recvd = recv(socket2->fd, buff, n, 0), -1, fail);
	// handle ntoh
	return b_recvd;
fail:
	return -1;
}

extern ssize_t socket2_srecv(const socket2_t handle, char** buff) {
	struct socket2* socket2 = handle;
	ssize_t b_recvd = 0;
	uint8_t* msg;
	uint8_t* p_msg;
	uint8_t chunk[CHUNK_SIZE] = { 0 };
	ssize_t cb_recvd;
	ssize_t tmp;
	size_t csize = CHUNK_SIZE / 2;
	bool end = false;
	try(msg = malloc(sizeof * msg), NULL, fail);
	do {
		try(cb_recvd = socket2_peek(socket2, chunk, CHUNK_SIZE), -1, fail);
		for (size_t i = 0; i < CHUNK_SIZE; i++) {
			if (!((char*)chunk)[i]) {
				csize = i + 1;
				end = true;
				break;
			}
		}
		try(p_msg = realloc(msg, sizeof * msg * (b_recvd + csize)), NULL, fail2);
		msg = p_msg;
		try(tmp = socket2_recv(socket2, msg + b_recvd, csize), -1, fail2);
		b_recvd += tmp;
		msg[b_recvd] = 0;
	} while (!end);
end:
	*buff = msg;
	return b_recvd;
fail2:
	free(msg);
fail:
	return -1;
}

extern ssize_t socket2_frecv(const socket2_t handle, FILE* file, long fsize) {
	struct socket2* socket2 = handle;
	ssize_t b_recvd = 0;
	uint8_t chunk[CHUNK_SIZE] = { 0 };
	FILE* ftmp = tmpfile();
	while (b_recvd < fsize) {
		ssize_t cb_recvd;
		try(cb_recvd = socket2_recv(socket2, chunk, fsize - b_recvd % CHUNK_SIZE), -1, fail);
		b_recvd += cb_recvd;
		fwrite(chunk, sizeof * chunk, cb_recvd, ftmp);
	}
	char c;
	rewind(ftmp);
	while ((c = fgetc(ftmp)) != EOF) {
		fputc(c, file);
	}
	fclose(ftmp);
	return b_recvd;
fail:
	return -1;
}

extern ssize_t socket2_send(const socket2_t handle, const uint8_t* buff, uint64_t n) {
	struct socket2* socket2 = handle;
	ssize_t b_sent;
	// TODO: hton conversion
	try(b_sent = send(socket2->fd, buff, n, 0), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern ssize_t socket2_ssend(const socket2_t handle, const char* string) {
	struct socket2* socket2 = handle;
	ssize_t b_sent;
	try(b_sent = socket2_send(socket2, string, strlen(string)), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern ssize_t socket2_fsend(const socket2_t handle, FILE* file) {
	struct socket2* socket2 = handle;
	ssize_t b_sent = 0;
	uint8_t chunk[CHUNK_SIZE] = {0};
	long flen;
	long cpos;
	fseek(file, 0L, SEEK_END);
	flen = ftell(file);
	fseek(file, 0L, SEEK_SET);
	do {
		// TODO: hton conversion
		long readed;
		try(readed = fread(chunk, sizeof * chunk, CHUNK_SIZE, file), -1, fail);
		try(b_sent += socket2_send(socket2, chunk, readed), -1, fail);
		try(cpos = ftell(file), -1, fail);
	} while (cpos != flen);
	return b_sent;
fail:
	return -1;
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
