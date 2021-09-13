#include "request_handler.h"

#include <stdlib.h>

#include "socket2.h"

#include "types/request_handler.h"

extern inline int cmn_request_handler_destroy(cmn_request_handler_t this) {
    int ret = this->__ops_vptr->destroy(this);
    if (!ret) {
        free(this);
    }
    return ret;
}

extern inline void cmn_request_handler_handle_request(cmn_request_handler_t this, cmn_socket2_t socket) {
    this->__ops_vptr->handle_request(this, socket);
}
