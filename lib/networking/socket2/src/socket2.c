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

extern int cmn_socket2_init(struct cmn_socket2* this, struct cmn_nproto_service* nproto_serivce, struct cmn_tproto_service* tproto_serivce) {
	memset(this, 0, sizeof * this);
	this->nproto_service = nproto_serivce;
	this->tproto_service = tproto_serivce;
	try(this->fd = socket(this->nproto_service->domain, this->tproto_service->type, this->tproto_service->protocol), INVALID_SOCKET, fail);
	return 0;
fail:
	return 1;
}

extern int cmn_socket2_destroy(struct cmn_socket2* this) {
	try(close(this->fd), -1, fail);
	free(this->address);
	return 0;
fail:
	return 1;
}

extern int cmn_socket2_accept(struct cmn_socket2* this, struct cmn_socket2* accepted) {
    cmn_socket2_init(accepted, this->nproto_service, this->tproto_service);
    try(accepted->address = malloc(this->addrlen), NULL, fail);
	while ((accepted->fd = accept(this->fd, accepted->address, &accepted->addrlen)) == -1) {
		try(errno != EMFILE, true, fail2);
		usleep(1);
	}
	return 0;
fail2:
	free(accepted->address);
	this->address = NULL;
fail:
	return 1;
}

extern int cmn_socket2_connect(struct cmn_socket2* this, const char* url) {
	try(cmn_nproto_service_set_address(this->nproto_service, this, url), 1, fail);
	try(connect(this->fd, this->address, this->addrlen), SOCKET_ERROR, fail2);
	return 0;
fail2:
	free(this->address);
	this->address = NULL;
fail:
	return -1;
}

extern int cmn_socket2_listen(struct cmn_socket2* this, const char* url, int backlog) {
	try(cmn_nproto_service_set_address(this->nproto_service, this, url), 1, fail);
	try(bind(this->fd, this->address, this->addrlen), -1, fail2);
	try(listen(this->fd, backlog), -1, fail2);
	return 0;
fail2:
	free(this->address);
	this->address = NULL;
fail:
	return 1;
}

extern ssize_t cmn_socket2_srecv(struct cmn_socket2* this, char** buff) {
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
		try(cb_recvd = cmn_socket2_peek(this, chunk, CHUNK_SIZE), -1, fail);
		for (size_t i = 0; i < CHUNK_SIZE; i++) {
			if (!((char*)chunk)[i]) {
				csize = i + 1;
				end = true;
				break;
			}
		}
		try(p_msg = realloc(msg, sizeof * msg * (b_recvd + csize)), NULL, fail2);
		msg = p_msg;
		try(tmp = cmn_socket2_recv(this, msg + b_recvd, csize), -1, fail2);
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

extern ssize_t cmn_socket2_frecv(struct cmn_socket2* this, FILE* file, long fsize) {
	ssize_t b_recvd = 0;
	uint8_t chunk[CHUNK_SIZE] = { 0 };
	FILE* ftmp = tmpfile();
	while (b_recvd < fsize) {
		ssize_t cb_recvd;
		try(cb_recvd = cmn_socket2_recv(this, chunk, fsize - b_recvd % CHUNK_SIZE), -1, fail);
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

extern ssize_t cmn_socket2_ssend(struct cmn_socket2* this, const char* string) {
	ssize_t b_sent;
	try(b_sent = cmn_socket2_send(this, string, strlen(string)), -1, fail);
	return b_sent;
fail:
	return -1;
}

extern ssize_t cmn_socket2_fsend(struct cmn_socket2* this, FILE* file) {
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
		try(b_sent += cmn_socket2_send(this, chunk, readed), -1, fail);
		try(cpos = ftell(file), -1, fail);
	} while (cpos != flen);
	return b_sent;
fail:
	return -1;
}

extern int cmn_socket2_get_fd(struct cmn_socket2* this) {
	return this->fd;
}

extern int cmn_socket2_set_blocking(struct cmn_socket2* this, bool blocking) {
	if (this->fd > 0) {
#ifdef _WIN32
		unsigned long mode = blocking ? 0 : 1;
		try(ioctlsocket(this->fd, FIONBIO, &mode), SOCKET_ERROR, fail);
		return 0;
#elif __unix__
		int flags;
		try (flags = fcntl(this->fd, F_GETFL, 0), -1, fail);
		flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
		try(fcntl(this->fd, F_SETFL, flags), !0, fail);
		return 0;
#endif
	}
fail:
	return 1;
}
