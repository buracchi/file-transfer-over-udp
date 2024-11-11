#ifndef BURACCHI_TFTP_CLIENT_H
#define BURACCHI_TFTP_CLIENT_H

#include <stdint.h>

#include <logger.h>
#include <tftp.h>

#include "client_stats.h"

struct tftp_client {
    struct logger *logger;
    void (*client_stats_callback)(struct tftp_client_stats stats[static 1]);
    uint8_t retries;
};

struct tftp_client_options {
    uint8_t timeout_s;
    uint16_t block_size;
    uint16_t window_size;
    bool use_tsize;
    bool use_adaptive_timeout;
};

struct tftp_client_server_address {
    const char *host;
    const char *port;
};

struct tftp_client_put_request {
    enum tftp_mode mode;
    const char *filename;
};

struct tftp_client_get_request {
    enum tftp_mode mode;
    const char *filename;
    const char *output;
};

struct tftp_client_response {
    bool success;
    bool server_may_not_support_options;
};

struct tftp_client_response tftp_client_list(struct tftp_client client[static 1],
                                             struct tftp_client_server_address server_address[static 1],
                                             struct tftp_client_options *options);

struct tftp_client_response tftp_client_get(struct tftp_client client[static 1],
                                            struct tftp_client_server_address server_address[static 1],
                                            struct tftp_client_get_request request[static 1],
                                            struct tftp_client_options *options);

struct tftp_client_response tftp_client_put(struct tftp_client client[static 1],
                                            struct tftp_client_server_address server_address[static 1],
                                            struct tftp_client_put_request request[static 1],
                                            struct tftp_client_options *options);

#endif // BURACCHI_TFTP_CLIENT_H
