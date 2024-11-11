#ifndef BURACCHI_TFTP_CLIENT_STATS_H
#define BURACCHI_TFTP_CLIENT_STATS_H

#include <time.h>

#include <logger.h>

struct tftp_client_stats {
    struct logger *logger;
    bool enabled;
    struct timespec start_time;
    struct timespec end_time;
    size_t file_bytes_received;
};

void tftp_client_stats_init(struct tftp_client_stats stats[static 1], struct logger logger[static 1]);

#endif // BURACCHI_TFTP_CLIENT_STATS_H
