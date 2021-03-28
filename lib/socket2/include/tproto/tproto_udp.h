#pragma once

#include "tproto.h"

struct tproto_udp {
    struct {
        struct tproto tproto;
    } super;
    struct tproto_udp_vtbl* __ops_vptr;
};

static struct tproto_udp_vtbl {
    int (*destroy)(struct tproto_udp* tproto);
} __tproto_udp_ops_vtbl = { 0 };

extern int tproto_udp_init(struct tproto_udp* tproto);

static inline int tproto_udp_destroy(struct tproto_udp* tproto) {
    return tproto->__ops_vptr->destroy(tproto);
}
