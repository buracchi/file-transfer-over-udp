#include "stats.h"

#include <errno.h>

#include <buracchi/tftp/client.h>

void stats_init(struct tftp_client_stats stats[static 1]) {
    *stats = (struct tftp_client_stats) {
        .file_bytes_transferred = 0,
    };
    if (clock_gettime(CLOCK_MONOTONIC, &stats->start_time.value) == -1) {
        stats->start_time.error_occurred = true;
        stats->start_time.error_number = errno;
    }
}
