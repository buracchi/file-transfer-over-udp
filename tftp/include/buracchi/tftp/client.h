#ifndef BURACCHI_TFTP_CLIENT_H
#define BURACCHI_TFTP_CLIENT_H

#include <stdint.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>

struct tftp_client_options {
    uint8_t *timeout_s;
    uint16_t *block_size;
    uint16_t *window_size;
    bool use_tsize;
    bool use_adaptive_timeout;
    bool is_read_type_list;
};

struct tftp_client_response {
    bool is_success;
    union {
        struct tftp_client_stats {
            struct {
                struct timespec value;
                union {
                    bool error_occurred;
                    int error_number;
                };
            } start_time;
            size_t file_bytes_transferred;
        } value;
        struct tftp_client_error {
            bool server_may_not_support_options;
        } error;
    };
};

struct tftp_client_response tftp_client_read(struct logger logger[static 1],
                                             uint8_t retries,
                                             const char host[static 1],
                                             const char port[static 1],
                                             const char filename[static 1],
                                             enum tftp_mode mode,
                                             struct tftp_client_options *options,
                                             FILE dest[static 1]);

struct tftp_client_response tftp_client_write(struct logger logger[static 1],
                                              uint8_t retries,
                                              const char host[static 1],
                                              const char port[static 1],
                                              const char filename[static 1],
                                              enum tftp_mode mode,
                                              struct tftp_client_options *options,
                                              FILE src[static 1]);

#endif // BURACCHI_TFTP_CLIENT_H
