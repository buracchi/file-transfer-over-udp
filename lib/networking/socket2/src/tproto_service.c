#include "tproto_service.h"

#include <stdint.h>

#include "types/tproto_service.h"

extern inline int cmn_tproto_service_accept(cmn_tproto_service_t this, cmn_socket2_t socket, struct sockaddr *addr,
                                            socklen_t *addr_len) {
    return this->__ops_vptr->accept(this, socket, addr, addr_len);
}

extern inline int cmn_tproto_service_connect(cmn_tproto_service_t this, cmn_socket2_t socket, struct sockaddr *addr,
                                             socklen_t addr_len) {
    return this->__ops_vptr->connect(this, socket, addr, addr_len);
}

extern inline int cmn_tproto_service_listen(cmn_tproto_service_t this, cmn_socket2_t socket, int backlog) {
    return this->__ops_vptr->listen(this, socket, backlog);
}

extern inline ssize_t cmn_tproto_service_peek(cmn_tproto_service_t this, cmn_socket2_t socket, uint8_t *buff,
                                              uint64_t n) {
    return this->__ops_vptr->peek(this, socket, buff, n);
}

extern inline ssize_t cmn_tproto_service_recv(cmn_tproto_service_t this, cmn_socket2_t socket, uint8_t *buff,
                                              uint64_t n) {
    return this->__ops_vptr->recv(this, socket, buff, n);
}

extern inline ssize_t cmn_tproto_service_send(cmn_tproto_service_t this, cmn_socket2_t socket, const uint8_t *buff,
                                              uint64_t n) {
    return this->__ops_vptr->send(this, socket, buff, n);
}

extern inline int cmn_tproto_service_close(cmn_tproto_service_t this, cmn_socket2_t socket) {
    return this->__ops_vptr->close(this, socket);
}
