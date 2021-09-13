#pragma once

#include "socket2.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_request_handler *cmn_request_handler_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern int cmn_request_handler_destroy(cmn_request_handler_t request_handler);

/**
* Always check is socket is not NULL before use it
*/
extern void cmn_request_handler_handle_request(cmn_request_handler_t request_handler, cmn_socket2_t socket);
