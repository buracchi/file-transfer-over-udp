#pragma once

#include <stdio.h>

typedef struct ft_service* ft_service_t;

extern ft_service_t ft_service_init(const char* base_dir);

extern int ft_service_destroy(const ft_service_t ft_service);

extern const char* ft_service_get_base_dir(const ft_service_t ft_service);

extern char* ft_service_get_filelist(const ft_service_t ft_service);
