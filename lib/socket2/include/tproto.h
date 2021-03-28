#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "struct_socket2.h"

struct tproto {
    int type;
    struct tproto_vtbl* __ops_vptr;
};

static struct tproto_vtbl {
    ssize_t(*peek)(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
    ssize_t (*recv)(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n);
    ssize_t (*send)(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n);
} __tproto_ops_vtbl = { 0 };

static inline int tproto_get_type(struct tproto* tproto) {
    return tproto->type;
}

static inline ssize_t tproto_peek(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    return tproto->__ops_vptr->peek(tproto, socket2, buff, n);
}

static inline ssize_t tproto_recv(struct tproto* tproto, const struct socket2* socket2, uint8_t* buff, uint64_t n) {
    return tproto->__ops_vptr->recv(tproto, socket2, buff, n);
}

static inline ssize_t tproto_send(struct tproto* tproto, const struct socket2* socket2, const uint8_t* buff, uint64_t n) {
    return tproto->__ops_vptr->send(tproto, socket2, buff, n);
}
