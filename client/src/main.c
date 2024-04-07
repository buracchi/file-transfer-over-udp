#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <logger.h>
#include <tftp.h>
#include <errno.h>

#include "tftp_client.h"
#include "tftp_client_stats.h"

struct arguments {
    const char *host;                       // IP address or hostname of the server to connect to
    const char *port;                       // port number to use for the connection
    enum client_command command;            // command to send to the server (e.g. "list", "get", "put")
    enum tftp_mode mode;                    // transfer mode to use for the file transfer
    const char *filename;                   // file to send or receive (used with the "get" and "put" commands)
    uint8_t retries;                        // number of retries to attempt before giving up
    uint8_t timeout_s;                      // duration of the timeout to use for the Go-Back N protocol, in seconds
    uint16_t block_size;                    // size of the data block to use for the file transfer
    uint16_t window_size;                   // size of the dispatch window to use for the Go-Back N protocol
    bool use_tsize;                         //
    bool adaptive_timeout;                  // flag to use an adaptive timeout calculated dynamically based on network delays
    double loss_probability;                // probability of packet loss to simulate
    enum logger_log_level verbose_level;    // verbose level to output additional information
};

static inline long double timespec_to_double(struct timespec timespec[static 1]) {
    int64_t sec_as_nsec = timespec->tv_sec * 1'000'000'000LL;
    return (sec_as_nsec + timespec->tv_nsec) / 1e9L;
}

static inline struct arguments get_arguments();
static void print_stats(struct tftp_client_stats stats[static 1]);

int main(int argc, char *argv[argc + 1]) {
    struct tftp_client client;
    struct arguments args = get_arguments();
    struct logger logger;
    if (!logger_init(&logger, logger_default_config)) {
        return EXIT_FAILURE;
    }
    logger.config.default_level = args.verbose_level;
    if (!tftp_client_init(&client,
                          (struct tftp_client_arguments) {
                              .host = args.host,
                              .port = args.port,
                              .command = args.command,
                              .mode = args.mode,
                              .filename = args.filename,
                              .retries = args.retries,
                              .timeout_s = args.timeout_s,
                              .block_size = args.block_size,
                              .window_size = args.window_size,
                              .use_tsize = args.use_tsize,
                              .adaptive_timeout = args.adaptive_timeout,
                              .loss_probability = args.loss_probability,
                              .client_stats_callback = print_stats,
                          },
                          &logger)) {
        return EXIT_FAILURE;
    }
    if (!tftp_client_start(&client) && !client.server_maybe_do_not_support_options) {
        tftp_client_destroy(&client);
        return EXIT_FAILURE;
    }
    logger_log_debug(client.logger, "Closing connection.");
    tftp_client_destroy(&client);
    if (client.server_maybe_do_not_support_options) {
        logger_log_warn(client.logger, "Server may not support options. Retrying without options.");
        if (!tftp_client_init(&client,
                              (struct tftp_client_arguments) {
                                  .host = args.host,
                                  .port = args.port,
                                  .command = args.command,
                                  .mode = args.mode,
                                  .filename = args.filename,
                                  .retries = args.retries,
                                  .loss_probability = args.loss_probability,
                                  .client_stats_callback = print_stats,
                              },
                              &logger)) {
            return EXIT_FAILURE;
        }
        if (!tftp_client_start(&client) && !client.server_maybe_do_not_support_options) {
            tftp_client_destroy(&client);
            return EXIT_FAILURE;
        }
        logger_log_debug(client.logger, "Closing connection.");
        tftp_client_destroy(&client);
    }
    return EXIT_SUCCESS;
}

static inline struct arguments get_arguments() {
    return (struct arguments) {
        .host = "::1",
        .port = "6969",
        .filename = "pippo",
        .mode = TFTP_MODE_OCTET,
        .command = CLIENT_COMMAND_GET,
        .retries = 5,
        .timeout_s = 0,
        .block_size = 0,
        .window_size = 0,
        .use_tsize = false,
        .adaptive_timeout = false,
        .loss_probability = 0.0,
        .verbose_level = LOGGER_LOG_LEVEL_ALL,
    };
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
    long double bit_speed = (long double) stats->file_bytes_received * 8 / duration;
    long double byte_speed = (long double) stats->file_bytes_received / duration;
    static const char *unit_prefix[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y"};
    long double display_speed = byte_speed;
    size_t prefix_index = 0;
    while (display_speed > 1024 && prefix_index < sizeof unit_prefix) {
        display_speed /= 1024;
        prefix_index++;
    }
    logger_log_info(stats->logger, "Received %zu bytes in %.3Lf seconds [%.2Lf %sB/s, %.0Lf bit/s]",
                    stats->file_bytes_received,
                    duration,
                    display_speed,
                    unit_prefix[prefix_index],
                    bit_speed);
}

/*
int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t *args) {
    argparser_t parser;
    argparser_t list_parser;
    argparser_t get_parser;
    argparser_t put_parser;
    try(parser = argparser_init(argc, argv), NULL, fail);
    argparser_set_description(parser, "File transfer client.");
    try(argparser_add_argument(parser, &args->host, { .name = "host", .help = "the file transfer server address" }), 1, fail);
    try(argparser_add_argument(parser, &args->port, { .flag = "p", .long_flag = "port", .default_value = "1234", .help = "specify the listening port number (the default value is \"1234\")" }), 1, fail);
    argparser_set_subparsers_options(parser, (struct argparser_subparsers_options){ .required = true, .title = "commands" });
    try(list_parser = argparser_add_subparser(parser, &args->command, "list", "List available file on the remote server"), NULL, fail);
    try(get_parser = argparser_add_subparser(parser, &args->command, "get", "Download a file from the remote server"), NULL, fail);
    try(argparser_add_argument(get_parser, &args->filename, { .name = "filename", .help = "The file to download" }), 1, fail);
    try(put_parser = argparser_add_subparser(parser, &args->command, "put", "Upload a file to the remote server"), NULL, fail);
    try(argparser_add_argument(put_parser, &args->filename, { .name = "filename", .help = "The file to upload" }), 1, fail);
    try(argparser_parse_args(parser), 1, fail);
    argparser_destroy(parser);
    return 0;
    fail:
    return 1;
}
*/
