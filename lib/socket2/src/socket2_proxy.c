#include "socket2_proxy.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utilities.h"
#include "try.h"

static struct socket2_proxy_vtbl* get_socket2_proxy_vtbl();
static void socket2_vtbl_init(struct socket2_vtbl** pvtbl);
static int _destroy(struct socket2_proxy* this);
static struct socket2* _accept(struct socket2* this);
static int _connect(struct socket2* this, const char* url);
static int _listen(struct socket2* this, const char* url, int backlog);

static struct socket2_vtbl super_ops;

extern void socket2_proxy_init(struct socket2_proxy* this, int fd, struct tproto* tproto, struct nproto* nproto, struct socket2* socket2) {
    memset(this, 0, sizeof * this);
    socket2_existing_socket_init(&this->super.socket2, fd, tproto, nproto);
    socket2_vtbl_init(&this->super.socket2.__ops_vptr);
    this->__ops_vptr = get_socket2_proxy_vtbl();
    this->socket2 = socket2;
}

static struct socket2_proxy_vtbl* get_socket2_proxy_vtbl() {
    struct socket2_proxy_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__socket2_proxy_ops_vtbl, sizeof * &__socket2_proxy_ops_vtbl)) {
        __socket2_proxy_ops_vtbl.destroy = _destroy;
    }
    return &__socket2_proxy_ops_vtbl;
}

static void socket2_vtbl_init(struct socket2_vtbl** pvtbl) {
    struct socket2_vtbl vtbl_zero = { 0 };
    if (!memcmp(&vtbl_zero, &__socket2_ops_vtbl, sizeof * &__socket2_ops_vtbl)) {
        if (pvtbl) {
            memcpy(&super_ops, *pvtbl, sizeof * &super_ops);
            memcpy(&__socket2_ops_vtbl, *pvtbl, sizeof * &__socket2_ops_vtbl);
        }
        __socket2_ops_vtbl.accept = _accept;
        __socket2_ops_vtbl.connect = _connect;
        __socket2_ops_vtbl.listen = _listen;
    }
    *pvtbl = &__socket2_ops_vtbl;
}

static int _destroy(struct socket2_proxy* this) {
    this->super.socket2.__ops_vptr->destroy(&this->super.socket2);
    socket2_destroy(this->socket2);
}

static struct socket2* _accept(struct socket2* socket2) {
    // tproto_user should handle this adding the lisening socket event
    // return a socket2_proxy with the accepted socket2
}

static int _connect(struct socket2* socket2, const char* url) {
    struct socket2_proxy* this = container_of(socket2, struct socket2_proxy, super.socket2);
    return super_ops.connect(this->socket2, url);
}

static int _listen(struct socket2* socket2, const char* url, int backlog) {
    struct socket2_proxy* this = container_of(socket2, struct socket2_proxy, super.socket2);
    return super_ops.listen(this->socket2, url, backlog);
}
