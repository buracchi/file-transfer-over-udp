#pragma once

#include "socket2.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_request_handler* cmn_request_handler_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

static inline void cmn_request_handler_handle_request(cmn_request_handler_t request_handler, cmn_socket2_t socket) {
    request_handler->__ops_vptr->handle_request(request_handler, socket);
}
