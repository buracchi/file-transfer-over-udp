#pragma once

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_socket2;

struct cmn_nproto_service {
    struct cmn_nproto_service_vtbl* __ops_vptr;
    int domain;
};

struct cmn_nproto_service_vtbl {
    int     (*set_address)  (struct cmn_nproto_service* service, struct cmn_socket2* socket, const char* url);
};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

static inline int cmn_nproto_service_set_address(struct cmn_nproto_service* service, struct cmn_socket2* socket, const char* url) {
    return service->__ops_vptr->set_address(service, socket, url);
}
