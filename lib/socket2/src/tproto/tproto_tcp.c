#include "tproto/tproto_tcp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "try.h"
#include "utilities.h"

static struct tproto_tcp_vtbl* get_tproto_tcp_vtbl();
static void tproto_vtbl_init(struct tproto_vtbl** pvtbl);
static int destroy(struct tproto_tcp* this);
static ssize_t _peek(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
static ssize_t _recv(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
static ssize_t _send(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n);

extern int tproto_tcp_init(struct tproto_tcp* this) {
    memset(this, 0, sizeof * this);
    tproto_vtbl_init(&this->super.tproto.__ops_vptr);
    this->__ops_vptr = get_tproto_tcp_vtbl();
    this->super.tproto.type = SOCK_STREAM;
    this->super.tproto.kernel_handled = true;
    return 0;
}

static struct tproto_tcp_vtbl* get_tproto_tcp_vtbl() {
    struct tproto_tcp_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__tproto_tcp_ops_vtbl, sizeof * &__tproto_tcp_ops_vtbl)) {
        __tproto_tcp_ops_vtbl.destroy = destroy;
    }
    return &__tproto_tcp_ops_vtbl;
}

static void tproto_vtbl_init(struct tproto_vtbl** pvtbl) {
    struct tproto_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__tproto_ops_vtbl, sizeof * &__tproto_ops_vtbl)) {
        __tproto_ops_vtbl.peek = _peek;
        __tproto_ops_vtbl.recv = _recv;
        __tproto_ops_vtbl.send = _send;
    }
    *pvtbl = &__tproto_ops_vtbl;
}

static int destroy(struct tproto_tcp* this) {
    return 0;
}

static ssize_t _peek(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    return recv(socket2->fd, buff, n, MSG_PEEK);
}

static ssize_t _recv(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    return recv(socket2->fd, buff, n, 0);
}

static ssize_t _send(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n) {
    return send(socket2->fd, buff, n, 0);
}
