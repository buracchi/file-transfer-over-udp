#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct cmn_socket2* cmn_socket2_t;

typedef struct cmn_tproto_service {
    struct cmn_tproto_service_vtbl* __ops_vptr;
    int type;
    int protocol;
} *cmn_tproto_service_t;

struct cmn_tproto_service_vtbl {
    ssize_t (*peek)     (cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t* buff, uint64_t n);
    ssize_t (*recv)     (cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t* buff, uint64_t n);
    ssize_t (*send)     (cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t* buff, uint64_t n);
};
