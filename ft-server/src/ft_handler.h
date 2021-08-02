#pragma once

#include "ft_service.h"

#include "socket2.h"

typedef void* ft_handler_t;

extern ft_handler_t ft_handler_init(const ft_service_t ft_service);

extern int ft_handler_destroy(const ft_handler_t ft_handler);
