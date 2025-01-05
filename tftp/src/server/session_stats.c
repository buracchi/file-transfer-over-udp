#include <buracchi/tftp/server_session_stats.h>

#include <sys/socket.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>

#include "../utils/inet.h"

struct tftp_session_stats tftp_session_stat_init(const struct sockaddr server_address[static 1],
                                                 const struct sockaddr peer_address[static 1],
                                                 const char file_path[static 1],
                                                 void (*callback)(struct tftp_session_stats *),
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
        .callback = callback,
    };
    sockaddr_ntop(server_address, stats.server_addr, &stats.server_port);
    sockaddr_ntop(peer_address, stats.peer_addr, &stats.peer_port);
    return stats;
}
