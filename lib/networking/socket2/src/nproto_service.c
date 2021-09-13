#include "nproto_service.h"

#include "types/nproto_service.h"

extern inline int cmn_nproto_service_set_address(cmn_nproto_service_t this, cmn_socket2_t socket, const char *url) {
    return this->__ops_vptr->set_address(this, socket, url);
}
