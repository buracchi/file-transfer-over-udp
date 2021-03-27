#pragma once

#include <sys/socket.h>

struct sockaddr2 {
    struct sockaddr* addr;
    socklen_t addrlen;
};
