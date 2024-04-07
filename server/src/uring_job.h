#pragma once

#include <stdint.h>

struct uring_job {
    void (*routine)(int32_t aio_result, void *);
    void* args;
};
