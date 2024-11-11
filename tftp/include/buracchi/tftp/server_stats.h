#ifndef TFTP_SERVER_STATS_H
#define TFTP_SERVER_STATS_H

#include <stdint.h>
#include <threads.h>
#include <netdb.h>

#include <logger.h>

struct tftp_server_stats_counters {
    uint64_t sessions_count;
};

struct tftp_server_stats {
    struct logger *logger;
    mtx_t mtx;
    time_t start_time;
    int interval;
    bool (*metrics_callback)(struct tftp_server_stats *);
    struct tftp_server_stats_counters counters;
};

bool tftp_server_stats_init(struct tftp_server_stats stats[static 1],
                            int interval,
                            bool (*metrics_callback)(struct tftp_server_stats *),
                            struct logger logger[static 1]);

void tftp_server_stats_destroy(struct tftp_server_stats stats[static 1]);

#endif // TFTP_SERVER_STATS_H
