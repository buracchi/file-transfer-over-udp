#include "tftp_server_stats.h"

#include <string.h>
#include <stdint.h>

bool tftp_server_stats_init(struct tftp_server_stats stats[static 1], int interval, struct logger logger[static 1]) {
    *stats = (struct tftp_server_stats) {
        .logger = logger,
        .interval = interval,
        .start_time = time(nullptr),
        .counters = {}
    };
    if (mtx_init(&stats->mtx, mtx_plain) != thrd_success) {
        return false;
    }
    return true;
}

void tftp_server_stats_destroy(struct tftp_server_stats stats[static 1]) {
    mtx_destroy(&stats->mtx);
}
