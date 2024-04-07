#pragma once

#include <stdint.h>
#include <threads.h>
#include <netdb.h>

#include "logger.h"

struct tftp_server_stats_counters {
    uint64_t sessions_count;
};

struct tftp_server_stats {
    struct logger *logger;
    int interval;
    time_t start_time;
    mtx_t mtx;
    struct tftp_server_stats_counters counters;
};

bool tftp_server_stats_init(struct tftp_server_stats stats[static 1], int interval, struct logger logger[static 1]);

void tftp_server_stats_destroy(struct tftp_server_stats stats[static 1]);
