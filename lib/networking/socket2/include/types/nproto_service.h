#pragma once

typedef struct cmn_socket2 *cmn_socket2_t;

typedef struct cmn_nproto_service {
    struct cmn_nproto_service_vtbl *__ops_vptr;
    int domain;
} *cmn_nproto_service_t;

struct cmn_nproto_service_vtbl {
    int (*set_address)(cmn_nproto_service_t service, cmn_socket2_t socket, const char *url);
};
