#include "socket2.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "struct_socket2.h"

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

static struct socket2_vtbl* get_socket2_vtbl();
static int _destroy(struct socket2* this);
static struct socket2* _accept(struct socket2* this);
static int _connect(struct socket2* this, const char* url);
static int _listen(struct socket2* this, const char* url, int backlog);
static ssize_t _peek(struct socket2* this, uint8_t* buff, uint64_t n);
static ssize_t _recv(struct socket2* this, uint8_t* buff, uint64_t n);
static ssize_t _send(struct socket2* this, const uint8_t* buff, uint64_t n);

extern int socket2_init(struct socket2* this, struct tproto* tproto, struct nproto* nproto) {
	memset(this, 0, sizeof * this);
	this->__ops_vptr = get_socket2_vtbl();
	this->tproto = tproto;
	this->nproto = nproto;
	int domain = nproto_get_domain(nproto);
	int type = tproto_get_type(tproto);
	try(this->fd = socket(domain, type, 0), INVALID_SOCKET, fail);
	return 0;
fail2:
	close(this->fd);
fail:
	free(this);
	return 1;
}

extern int socket2_existing_socket_init(struct socket2* this, int fd, struct tproto* tproto, struct nproto* nproto) {
	memset(this, 0, sizeof * this);
	this->__ops_vptr = get_socket2_vtbl();
	this->tproto = tproto;
	this->nproto = nproto;
	this->fd = fd;
	return 0;
}

static struct socket2_vtbl* get_socket2_vtbl() {
	struct socket2_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__socket2_ops_vtbl, sizeof * &__socket2_ops_vtbl)) {
		__socket2_ops_vtbl.destroy = _destroy;
		__socket2_ops_vtbl.accept = _accept;
		__socket2_ops_vtbl.connect = _connect;
		__socket2_ops_vtbl.listen = _listen;
		__socket2_ops_vtbl.peek = _peek;
		__socket2_ops_vtbl.recv = _recv;
		__socket2_ops_vtbl.send = _send;
	}
	return &__socket2_ops_vtbl;
}

static int _destroy(struct socket2* this) {
	try(close(this->fd), -1, fail);
	if (this->address) {
		try(delete(this->address), 1, fail);
	}
	return 0;
fail:
	return 1;
}

static struct socket2* _accept(struct socket2* this) {
	struct socket2* accepted;
	accepted = malloc(sizeof * accepted);
	if (accepted) {
		accepted->nproto = this->nproto;
		accepted->tproto = this->tproto;
		accepted->address = malloc(sizeof * accepted->address);
		accepted->address->addr = malloc(this->address->addrlen);
		accepted->address->addrlen = this->address->addrlen;
		accepted->address->__ops_vptr = this->address->__ops_vptr;
		accepted->__ops_vptr = &__socket2_ops_vtbl;
		while ((accepted->fd = accept(this->fd, accepted->address->addr, &accepted->address->addrlen)) == -1) {
			try(errno != EMFILE, true, fail);
			sleep(0.1);
		}
	}
	return accepted;
fail:
	free(accepted);
	return NULL;
}

static int _connect(struct socket2* this, const char* url) {
	try(this->address = nproto_get_sockaddr2(this->nproto, url), NULL, fail);
	try(connect(this->fd, this->address->addr, this->address->addrlen), SOCKET_ERROR, fail2);
	return 0;
fail2:
	delete(this->address);
fail:
	return -1;
}

static int _listen(struct socket2* this, const char* url, int backlog) {
	try(this->address = nproto_get_sockaddr2(this->nproto, url), NULL, fail);
	try(bind(this->fd, this->address->addr, this->address->addrlen), -1, fail);
	try(listen(this->fd, backlog), -1, fail);
	return 0;
fail:
	return 1;
}

static ssize_t _peek(struct socket2* this, uint8_t* buff, uint64_t n) {
	return tproto_peek(this->tproto, this, buff, n);
}

static ssize_t _recv(struct socket2* this, uint8_t* buff, uint64_t n) {
	ssize_t b_recvd;
	try(b_recvd = tproto_recv(this->tproto, this, buff, n), -1, fail);
	// handle ntoh
	return b_recvd;
fail:
	return -1;
}

static ssize_t _send(struct socket2* this, const uint8_t* buff, uint64_t n) {
	ssize_t b_sent;
	// TODO: hton conversion
	try(b_sent = tproto_send(this->tproto, this, buff, n), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern ssize_t socket2_srecv(struct socket2* handle, char** buff) {
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

extern ssize_t socket2_frecv(struct socket2* handle, FILE* file, long fsize) {
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

extern ssize_t socket2_ssend(struct socket2* handle, const char* string) {
	struct socket2* socket2 = handle;
	ssize_t b_sent;
	try(b_sent = socket2_send(socket2, string, strlen(string)), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern ssize_t socket2_fsend(struct socket2* handle, FILE* file) {
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

extern int socket2_get_fd(struct socket2* handle) {
	struct socket2* socket2 = handle;
	return socket2->fd;
}

extern int socket2_set_blocking(struct socket2* handle, bool blocking) {
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
