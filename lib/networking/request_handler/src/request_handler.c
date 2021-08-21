#include "request_handler.h"

#include "socket2.h"

#include "types/request_handler.h"

extern inline void cmn_request_handler_handle_request(cmn_request_handler_t request_handler, cmn_socket2_t socket) {
    request_handler->__ops_vptr->handle_request(request_handler, socket);
}
