#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>
#include <errno.h>

#include <buracchi/tftp/client.h>
#include <buracchi/tftp/client_stats.h>

#include "client_cli.h"

static inline long double timespec_to_double(struct timespec timespec[static 1]) {
    static const int64_t nanoseconds_per_second_ll = 1000000000LL;
    static const long double nanoseconds_per_second_ld = 1e9L;
    int64_t sec_as_nsec = timespec->tv_sec * nanoseconds_per_second_ll;
    return (sec_as_nsec + timespec->tv_nsec) / nanoseconds_per_second_ld;
}

static void print_stats(struct tftp_client_stats stats[static 1]);

int main(int argc, char *argv[static argc + 1]) {
    struct cli_args args;
    struct logger logger;
    if (!cli_args_parse(&args, argc, (const char **) argv)) {
        return EXIT_FAILURE;
    }
    if (!logger_init(&logger, logger_default_config)) {
        return EXIT_FAILURE;
    }
    logger.config.default_level = args.verbose_level;
    struct tftp_client_options options = (struct tftp_client_options) {
        .timeout_s = args.options.timeout_s,
        .block_size = args.options.block_size,
        .window_size = args.options.window_size,
        .use_tsize = args.options.use_tsize,
        .use_adaptive_timeout = args.options.adaptive_timeout,
    };
    struct tftp_client client = {
        .logger = &logger,
        .client_stats_callback = print_stats,
        .retries = args.retries,
    };
    struct tftp_client_response response = {};
    bool is_retransmission = false;
    do {
        struct tftp_client_options *options_ptr = is_retransmission ? nullptr : &options;
        if (is_retransmission) {
            logger_log_warn(client.logger, "Server may not support options. Retrying without options.");
        }
        switch (args.command) {
            case CLIENT_COMMAND_LIST:
                response = tftp_client_list(&client,
                                            &(struct tftp_client_server_address) {
                                                .host = args.host,
                                                .port = args.port,
                                            },
                                            options_ptr);
                break;
            case CLIENT_COMMAND_GET:
                response = tftp_client_get(&client,
                                           &(struct tftp_client_server_address) {
                                               .host = args.host,
                                               .port = args.port,
                                           },
                                           &(struct tftp_client_get_request) {
                                               .mode = args.command_args.get.mode,
                                               .filename = args.command_args.get.filename,
                                               .output = args.command_args.get.output,
                                           },
                                           options_ptr);
                break;
            case CLIENT_COMMAND_PUT:
                response = tftp_client_put(&client,
                                           &(struct tftp_client_server_address) {
                                               .host = args.host,
                                               .port = args.port,
                                           },
                                           &(struct tftp_client_put_request) {
                                               .mode = args.command_args.put.mode,
                                               .filename = args.command_args.put.filename,
                                           },
                                           options_ptr);
        }
        if (!response.success && !response.server_may_not_support_options) {
            cli_args_destroy(&args);
            return EXIT_FAILURE;
        }
        is_retransmission = true;
    } while (!response.success);
    logger_log_debug(client.logger, "Closing connection.");
    cli_args_destroy(&args);
    return EXIT_SUCCESS;
}

static void print_stats(struct tftp_client_stats stats[static 1]) {
    if (clock_gettime(CLOCK_MONOTONIC, &stats->end_time) == -1) {
        logger_log_warn(stats->logger, "Failed to get end time. %s", strerror(errno));
        logger_log_warn(stats->logger, "Stats will be disabled.");
        stats->enabled = false;
        return;
    }
    long double duration = timespec_to_double(&(struct timespec) {
        .tv_sec = stats->end_time.tv_sec - stats->start_time.tv_sec,
        .tv_nsec = stats->end_time.tv_nsec - stats->start_time.tv_nsec,
    });
    static const int bits_per_byte = 8;
    long double bit_speed = (long double) stats->file_bytes_received * bits_per_byte / duration;
    long double byte_speed = (long double) stats->file_bytes_received / duration;
    static const char *unit_prefix[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y"};
    static const int unit_step = 1000;
    long double display_speed = byte_speed;
    size_t prefix_index = 0;
    while (display_speed > unit_step && prefix_index < sizeof unit_prefix) {
        display_speed /= unit_step;
        prefix_index++;
    }
    logger_log_info(stats->logger, "Received %zu bytes in %.3Lf seconds [%.2Lf %sB/s, %.0Lf bit/s]",
                    stats->file_bytes_received,
                    duration,
                    display_speed,
                    unit_prefix[prefix_index],
                    bit_speed);
}
