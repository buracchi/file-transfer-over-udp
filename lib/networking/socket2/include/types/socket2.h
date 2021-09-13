#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "nproto_service.h"
#include "tproto_service.h"

struct cmn_socket2 {
    int fd;
    bool is_non_block;
    bool is_cloexec;
    socklen_t addrlen;
    struct sockaddr *address;
    cmn_nproto_service_t nproto_service;
    cmn_tproto_service_t tproto_service;
};
