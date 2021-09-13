#include "socket2.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "types/socket2.h"
#include "try.h"

#ifdef __unix__

#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <errno.h>

#endif

#define INVALID_SOCKET -1
#define SOCKET_ERROR  -1
#define CHUNK_SIZE 4096    // MUST be even

extern cmn_socket2_t cmn_socket2_init(cmn_nproto_service_t nproto_serivce, cmn_tproto_service_t tproto_serivce) {
    cmn_socket2_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    memset(this, 0, sizeof *this);
    this->nproto_service = nproto_serivce;
    this->tproto_service = tproto_serivce;
    int domain = this->nproto_service->domain;
    int type = this->tproto_service->type;
    int protocol = this->tproto_service->protocol;
    try(this->fd = socket(domain, type, protocol), INVALID_SOCKET, fail2);
    try(setsockopt(this->fd, SOL_SOCKET, SO_LINGER, &(struct linger) {.l_onoff=1, .l_linger=0}, sizeof(struct linger)),
        -1, fail3);
    return this;
fail3:
    close(this->fd);
fail2:
    free(this);
fail:
    return NULL;
}

extern int cmn_socket2_destroy(cmn_socket2_t this) {
    try(cmn_tproto_service_close(this->tproto_service, this), -1, fail);
    free(this->address);
    free(this);
    return 0;
fail:
    return 1;
}

extern cmn_socket2_t cmn_socket2_accept(cmn_socket2_t this) {
    cmn_socket2_t accepted;
    try(accepted = cmn_socket2_init(this->nproto_service, this->tproto_service), NULL, fail);
    try(accepted->address = malloc(this->addrlen), NULL, fail2);
    while ((accepted->fd = cmn_tproto_service_accept(this->tproto_service, this, accepted->address,
                                                     &accepted->addrlen)) == -1) {
        try(errno != EMFILE, true, fail3);
        usleep(1);
    }
    return accepted;
fail3:
    free(accepted->address);
    this->address = NULL;
fail2:
    cmn_socket2_destroy(accepted);
fail:
    return NULL;
}

extern int cmn_socket2_connect(cmn_socket2_t this, const char *url) {
    try(cmn_nproto_service_set_address(this->nproto_service, this, url), 1, fail);
    try(cmn_tproto_service_connect(this->tproto_service, this, this->address, this->addrlen), SOCKET_ERROR, fail2);
    return 0;
fail2:
    free(this->address);
    this->address = NULL;
fail:
    return -1;
}

extern int cmn_socket2_listen(cmn_socket2_t this, const char *url, int backlog) {
    try(cmn_nproto_service_set_address(this->nproto_service, this, url), 1, fail);
    try(bind(this->fd, this->address, this->addrlen), -1, fail2);
    try(cmn_tproto_service_listen(this->tproto_service, this, backlog), -1, fail2);
    return 0;
fail2:
    free(this->address);
    this->address = NULL;
fail:
    return 1;
}

extern inline ssize_t cmn_socket2_peek(cmn_socket2_t this, uint8_t *buff, uint64_t n) {
    return cmn_tproto_service_peek(this->tproto_service, this, buff, n);
}

extern inline ssize_t cmn_socket2_recv(cmn_socket2_t this, uint8_t *buff, uint64_t n) {
    return cmn_tproto_service_recv(this->tproto_service, this, buff, n);
}

extern ssize_t cmn_socket2_srecv(cmn_socket2_t this, char **buff) {
    ssize_t b_recvd = 0;
    uint8_t *msg;
    uint8_t *p_msg;
    uint8_t chunk[CHUNK_SIZE] = {0};
    ssize_t cb_recvd;
    ssize_t tmp;
    size_t csize = CHUNK_SIZE / 2;
    bool end = false;
    try(msg = malloc(sizeof *msg), NULL, fail);
    do {
        try(cb_recvd = cmn_socket2_peek(this, chunk, CHUNK_SIZE), -1, fail);
        for (size_t i = 0; i < CHUNK_SIZE; i++) {
            if (!((char *) chunk)[i]) {
                csize = i + 1;
                end = true;
                break;
            }
        }
        try(p_msg = realloc(msg, sizeof *msg * (b_recvd + csize)), NULL, fail2);
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

extern ssize_t cmn_socket2_frecv(cmn_socket2_t this, FILE *file, long fsize) {
    ssize_t b_recvd = 0;
    uint8_t chunk[CHUNK_SIZE] = {0};
    FILE *ftmp = tmpfile();
    while (b_recvd < fsize) {
        ssize_t cb_recvd;
        try(cb_recvd = cmn_socket2_recv(this, chunk, fsize - b_recvd % CHUNK_SIZE), -1, fail);
        b_recvd += cb_recvd;
        fwrite(chunk, sizeof *chunk, cb_recvd, ftmp);
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

extern inline ssize_t cmn_socket2_send(cmn_socket2_t this, const uint8_t *buff, uint64_t n) {
    return cmn_tproto_service_send(this->tproto_service, this, buff, n);
}

extern ssize_t cmn_socket2_ssend(cmn_socket2_t this, const char *string) {
    ssize_t b_sent;
    try(b_sent = cmn_socket2_send(this, string, strlen(string)), -1, fail);
    return b_sent;
fail:
    return -1;
}

extern ssize_t cmn_socket2_fsend(cmn_socket2_t this, FILE *file) {
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
        try(readed = fread(chunk, sizeof *chunk, CHUNK_SIZE, file), -1, fail);
        try(b_sent += cmn_socket2_send(this, chunk, readed), -1, fail);
        try(cpos = ftell(file), -1, fail);
    } while (cpos != flen);
    return b_sent;
fail:
    return -1;
}

extern int cmn_socket2_get_fd(cmn_socket2_t this) {
    return this->fd;
}

extern int cmn_socket2_set_blocking(cmn_socket2_t this, bool blocking) {
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
