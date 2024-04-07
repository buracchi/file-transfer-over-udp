#pragma once

#include <time.h>

#include <logger.h>

struct tftp_client_stats {
    struct logger *logger;
    bool enabled;
    struct timespec start_time;
    struct timespec end_time;
    size_t file_bytes_received;
};

void tftp_client_stats_init(struct tftp_client_stats stats[1], struct logger logger[1]);
