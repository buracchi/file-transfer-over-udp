#include "tproto_service.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "types/tproto_service.h"

extern inline ssize_t cmn_tproto_service_peek(cmn_tproto_service_t this, cmn_socket2_t socket, uint8_t* buff, uint64_t n) {
    return this->__ops_vptr->peek(this, socket, buff, n);
}

extern inline ssize_t cmn_tproto_service_recv(cmn_tproto_service_t this, cmn_socket2_t socket, uint8_t* buff, uint64_t n) {
    return this->__ops_vptr->recv(this, socket, buff, n);
}

extern inline ssize_t cmn_tproto_service_send(cmn_tproto_service_t this, cmn_socket2_t socket, const uint8_t* buff, uint64_t n) {
    return this->__ops_vptr->send(this, socket, buff, n);
}
