#pragma once

#include <buracchi/common/networking/request_handler.h>

typedef struct ft_service* ft_service_t;

extern ft_service_t ft_service_init(const char* base_dir);

extern cmn_request_handler_t ft_service_get_request_handler(ft_service_t ft_service);

extern int ft_service_destroy(ft_service_t ft_service);
