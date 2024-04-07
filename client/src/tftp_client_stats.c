#include "tftp_client_stats.h"

#include <errno.h>
#include <string.h>

void tftp_client_stats_init(struct tftp_client_stats stats[1], struct logger logger[1]) {
    *stats = (struct tftp_client_stats) {
        .enabled = true,
        .logger = logger,
        .file_bytes_received = 0,
    };
    if (clock_gettime(CLOCK_MONOTONIC, &stats->start_time) == -1) {
        logger_log_warn(logger, "Failed to get start time. %s", strerror(errno));
        logger_log_warn(logger, "Stats will be disabled.");
        stats->enabled = false;
    }
}
