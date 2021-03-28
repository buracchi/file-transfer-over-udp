#pragma once

#include "tproto.h"

struct tproto_tcp {
    struct {
        struct tproto tproto;
    } super;
    struct tproto_tcp_vtbl* __ops_vptr;
};

static struct tproto_tcp_vtbl {
    int (*destroy)(struct tproto_tcp* tproto);
} __tproto_tcp_ops_vtbl = { 0 };

extern int tproto_tcp_init(struct tproto_tcp* tproto);

static inline int tproto_tcp_destroy(struct tproto_tcp* tproto) {
    return tproto->__ops_vptr->destroy(tproto);
}
