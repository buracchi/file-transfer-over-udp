#include "session_stats.h"
#include "inet_utils.h"

#include "tftp.h"

struct tftp_session_stats tftp_session_stat_init(const struct sockaddr server_address[static 1],
                                                 const struct sockaddr peer_address[static 1],
                                                 const char file_path[static 1],
                                                 struct logger logger[static 1]) {
    struct tftp_session_stats stats = {
        .logger = logger,
        .file_path = file_path,
        .error = {.error_occurred = false, .error_number = 0, .error_message = nullptr},
        .options_acked = "[]",
        .start_time = time(nullptr),
        .packets_sent = 0,
        .packets_acked = 0,
        .bytes_sent = 0,
        .retransmits = 0,
        .blksize = tftp_default_blksize,
        .window_size = tftp_default_window_size,
    };
    inet_ntop_address(server_address, stats.server_addr, &stats.server_port);
    inet_ntop_address(peer_address, stats.peer_addr, &stats.peer_port);
    return stats;
}
