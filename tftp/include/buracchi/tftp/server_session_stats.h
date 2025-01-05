#ifndef TFTP_SESSION_STATS_H
#define TFTP_SESSION_STATS_H

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>

struct tftp_session_stats {
    struct logger *logger;
    char peer_addr[INET6_ADDRSTRLEN];
    uint16_t peer_port;
    char server_addr[INET6_ADDRSTRLEN];
    uint16_t server_port;
    const char *file_path;
    const char *mode;
    struct tftp_session_stats_error {
        bool error_occurred;
        bool is_non_terminating;
        uint16_t error_number;
        const char *error_message;
    } error;
    char options_in[tftp_option_formatted_string_max_size];
    char options_acked[tftp_option_formatted_string_max_size];
    time_t start_time;
    int packets_sent;
    int packets_acked;
    size_t bytes_sent;
    int retransmits;
    uint16_t blksize;
    uint16_t window_size;
    void (*callback)(struct tftp_session_stats *);
};

struct tftp_session_stats tftp_session_stat_init(const struct sockaddr server_address[static 1],
                                                 const struct sockaddr peer_address[static 1],
                                                 const char file_path[static 1],
                                                 void (*callback)(struct tftp_session_stats *),
                                                 struct logger logger[static 1]);

#endif // TFTP_SESSION_STATS_H
