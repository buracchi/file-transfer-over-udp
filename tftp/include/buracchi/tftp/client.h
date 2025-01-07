#ifndef BURACCHI_TFTP_CLIENT_H
#define BURACCHI_TFTP_CLIENT_H

#include <stdint.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>

struct tftp_client_stats {
    struct logger *logger;
    bool enabled;
    struct timespec start_time;
    struct timespec end_time;
    size_t file_bytes_received;
};

struct tftp_client {
    struct logger *logger;
    void (*client_stats_callback)(struct tftp_client_stats stats[static 1]);
    uint8_t retries;
};

struct tftp_client_options {
    uint8_t *timeout_s;
    uint16_t *block_size;
    uint16_t *window_size;
    bool use_tsize;
    bool use_adaptive_timeout;
};

struct tftp_client_response {
    bool success;
    bool server_may_not_support_options;
};

struct tftp_client_response tftp_client_list(struct tftp_client client[static 1],
                                             const char host[static 1],
                                             const char port[static 1],
                                             const char directory[static 1],
                                             enum tftp_mode mode,
                                             struct tftp_client_options options[static 1],
                                             FILE dest[static 1]);

struct tftp_client_response tftp_client_get(struct tftp_client client[static 1],
                                            const char host[static 1],
                                            const char port[static 1],
                                            const char filename[static 1],
                                            enum tftp_mode mode,
                                            struct tftp_client_options *options,
                                            FILE dest[static 1]);

struct tftp_client_response tftp_client_put(struct tftp_client client[static 1],
                                            const char host[static 1],
                                            const char port[static 1],
                                            const char filename[static 1],
                                            enum tftp_mode mode,
                                            struct tftp_client_options *options,
                                            FILE src[static 1]);

#endif // BURACCHI_TFTP_CLIENT_H
