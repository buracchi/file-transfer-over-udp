#pragma once

#include "socket2.h"

typedef void* request_handler_t;

extern void request_handler_handle_request(request_handler_t request_handler, struct socket2* socket);
