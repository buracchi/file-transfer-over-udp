#include <buracchi/tftp/server_stats.h>

#include <threads.h>
#include <time.h>

#include <logger.h>

bool tftp_server_stats_init(struct tftp_server_stats stats[static 1],
                            int interval,
                            bool (*metrics_callback)(struct tftp_server_stats *),
                            struct logger logger[static 1]) {
    *stats = (struct tftp_server_stats) {
        .logger = logger,
        .interval = interval,
        .start_time = time(nullptr),
        .metrics_callback = metrics_callback,
        .counters = {},
    };
    return mtx_init(&stats->mtx, mtx_plain) == thrd_success;
}

void tftp_server_stats_destroy(struct tftp_server_stats stats[static 1]) {
    mtx_destroy(&stats->mtx);
}
