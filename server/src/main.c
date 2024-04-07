#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tftp_server.h"
#include "logger.h"

struct arguments {
    char *host;                             // host to listen on
    char *port;                             // port to listen on
    char *root;                             // file root directory
    uint32_t workers;                       // number of worker threads to use
    uint16_t max_worker_sessions;           // maximum number of sessions a worker can handle
    uint8_t retries;                        // number of retries to attempt before giving up
    uint8_t timeout_s;                      // duration of the timeout in seconds
    bool adaptive_timeout;                  // flag to use an adaptive timeout calculated dynamically based on network delays
    double loss_probability;                // probability of packet loss to simulate
    enum logger_log_level verbose_level;    // verbose level to output additional information
};

static void sigint_handler(int signal);
static inline struct arguments get_arguments();
static void print_session_stats(struct tftp_session_stats stats[static 1]);
static void print_server_stats(struct tftp_server_stats stats[static 1]);

static struct tftp_server server;

/*
typedef struct arguments {
    char *listen_port;         // port number to listen on
    char *directory;           // file root directory
    unsigned int thread_count; // number of worker threads to use
    uint32_t window_size;      // size of the dispatch window to use for the Go-Back N protocol
    long timeout_duration;     // duration of the timeout to use for the Go-Back N protocol, in milliseconds
    bool adaptive_timeout;     // flag to use an adaptive timeout calculated dynamically based on network delays
    double loss_probability;   // probability of packet loss to simulate
    int verbose_level;         // verbose level to output additional information
} cmdline_args_t;

int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t args[static 1], const char* cwd) {
	argparser_t argparser;
	try(argparser = argparser_init(argc, argv), NULL, fail);
	argparser_set_description(argparser, "File transfer server.");
	try(argparser_add_argument(argparser, &(args->listen_port), { .flag = "p", .long_flag = "port", .default_value = "1234", .help = "specify the listening port number (the default value is \"1234\")" }), 1, fail);
	try(argparser_add_argument(argparser, &(args->directory), { .flag = "d", .long_flag = "directory", .default_value = cwd, .help = "specify the shared directory (the default value is the current working directory)" }), 1, fail);
    try(argparser_add_argument(argparser, &(args->thread_count), { .flag = "t", .long_flag = "threads", .default_value = "4", .help = "specify the number of worker threads to use (the default value is 4)" }), 1, fail);
    try(argparser_add_argument(argparser, &(args->window_size), { .flag = "w", .long_flag = "window-size", .default_value = "4", .help = "specify the size of the dispatch window to use for the Go-Back N protocol (the default value is 4)" }), 1, fail);
    try(argparser_add_argument(argparser, &(args->timeout_duration), { .flag = "T", .long_flag = "timeout", .default_value = "1000", .help = "specify the duration of the timeout to use for the Go-Back N protocol, in milliseconds (the default value is 1000)" }), 1, fail);
    try(argparser_add_argument(argparser, &(args->verbose_level), { .flag = "v", .long_flag = "verbose", .default_value = "0", .help = "specify the verbose level to output additional information (the default value is 0)" }), 1, fail);
	try(argparser_parse_args(argparser), 1, fail);
	argparser_destroy(argparser);
	return 0;
fail:
	return 1;
}
*/

int main(int argc, char *argv[argc + 1]) {
    /*
    char cwd[PATH_MAX + 1];
    cmdline_args_t args;
    if (getcwd(cwd, sizeof cwd) == nullptr) {
        cmn_logger_log_error("Couldn't get the current working directory.");
        try(parse_cmdline_args(argc, (const char**)argv, &args, nullptr), 1, argparser_fail);
    }
    try(parse_cmdline_args(argc, (const char**)argv, &args, cwd), 1, argparser_fail);
    */
    enum { OLD, NEW };
    struct sigaction action[2] = {};
    struct arguments args = get_arguments();
    struct logger logger;
    if (!logger_init(&logger, logger_default_config)) {
        return EXIT_FAILURE;
    }
    logger.config.default_level = args.verbose_level;
    if (sigaction(SIGINT, nullptr, &action[OLD]) == -1) {
        logger_log_error(&logger, "Could not get the current SIGINT handler. %s", strerror(errno));
        return EXIT_FAILURE;
    }
    memcpy(&action[NEW], &action[OLD], sizeof *action);
    action[NEW].sa_handler = sigint_handler;
    bool server_initialized = tftp_server_init(&server,
                                               (struct tftp_server_arguments) {
                                                   .ip = args.host,
                                                   .port = args.port,
                                                   .root = args.root,
                                                   .retries = args.retries,
                                                   .timeout_s = args.timeout_s,
                                                   .workers = args.workers,
                                                   .max_worker_sessions = args.max_worker_sessions,
                                                   .server_stats_callback = print_server_stats,
                                                   .handler_stats_callback = print_session_stats,
                                                   .stats_interval_seconds = datapoints_interval_seconds,
                                               },
                                               &logger);
    if (!server_initialized) {
        logger_log_error(&logger, "Could not initialize server.");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGINT, &action[NEW], nullptr) == -1) {
        logger_log_error(&logger, "Could not set the new SIGINT handler. %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if (!tftp_server_start(&server)) {
        logger_log_error(&logger, "Server encountered an error.");
    }
    if (sigaction(SIGINT, &action[OLD], nullptr) == -1) {
        logger_log_error(&logger, "Could not restore the old SIGINT handler. %s", strerror(errno));
        return EXIT_FAILURE;
    }
    tftp_server_destroy(&server);
    return EXIT_SUCCESS;
}

static void sigint_handler([[maybe_unused]] int signal) {
    tftp_server_stop(&server);
}

static inline struct arguments get_arguments() {
    return (struct arguments) {
            .host = "::",
            .port = "6969",
            .retries = 5,
            .timeout_s = 2,
            .root = "",
            .workers = 7,
            .max_worker_sessions = 64,
            .verbose_level = LOGGER_LOG_LEVEL_DEBUG,
    };
}

static void print_session_stats(struct tftp_session_stats stats[static 1]) {
    logger_log_info(stats->logger, "Stats: for %s requesting %s [%s]", stats->peer_addr, stats->file_path, stats->mode);
    if (stats->error.error_occurred) {
        logger_log_warn(stats->logger, "\tError: %s", stats->error.error_message);
    }
    logger_log_info(stats->logger, "\tTime spent: %dms", (int) (difftime(time(nullptr), stats->start_time) * 1000));
    logger_log_info(stats->logger, "\tPackets sent: %d", stats->packets_sent);
    logger_log_info(stats->logger, "\tPackets ACKed: %d", stats->packets_acked);
    logger_log_info(stats->logger, "\tBytes sent: %zu", stats->bytes_sent);
    logger_log_info(stats->logger, "\tOptions: %s", stats->options_acked);
    logger_log_info(stats->logger, "\tBlock size: %zu", stats->blksize);
    logger_log_info(stats->logger, "\tWindow size: %d", stats->window_size);
    logger_log_info(stats->logger, "\tRetransmits: %d", stats->retransmits);
    logger_log_info(stats->logger, "\tServer port: %d", stats->server_port);
    logger_log_info(stats->logger, "\tClient port: %d", stats->peer_port);
}

static void print_server_stats(struct tftp_server_stats stats[static 1]) {
    struct tftp_server_stats_counters counters;
    mtx_lock(&stats->mtx);
    counters = stats->counters;
    stats->counters = (struct tftp_server_stats_counters) {};
    mtx_unlock(&stats->mtx);
    logger_log_info(stats->logger, "Server stats - every %d seconds", stats->interval);
    logger_log_info(stats->logger, "Number of spawned TFTP sessions in stats time frame : %lu", counters.sessions_count);
}
