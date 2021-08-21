#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_socket2;

struct cmn_tproto_service {
    struct cmn_tproto_service_vtbl* __ops_vptr;
    int type;
    int protocol;
};

struct cmn_tproto_service_vtbl {
    ssize_t (*peek)     (struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
    ssize_t (*recv)     (struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n);
    ssize_t (*send)     (struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n);
};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

static inline ssize_t cmn_tproto_service_peek(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return service->__ops_vptr->peek(service, socket, buff, n);
}

static inline ssize_t cmn_tproto_service_recv(struct cmn_tproto_service* service, struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return service->__ops_vptr->recv(service, socket, buff, n);
}

static inline ssize_t cmn_tproto_service_send(struct cmn_tproto_service* service, struct cmn_socket2* socket, const uint8_t* buff, uint64_t n) {
    return service->__ops_vptr->send(service, socket, buff, n);
}
