#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include <logger.h>
#include <buracchi/tftp/server.h>
#include <buracchi/tftp/server_stats.h>
#include <buracchi/tftp/server_session_stats.h>

#include "server_cli.h"
#include "server_packet_loss.h"

#define strerror_max_size 64
#define strerror_rbs(error_code) strerror_r_no_fail(error_code, (char[strerror_max_size]){})

static inline char* strerror_r_no_fail(int error_code, char buffer[static strerror_max_size]) {
    if (strerror_r(error_code, buffer, strerror_max_size) != nullptr) {
        snprintf(buffer, strerror_max_size, "strerror_r failed, error code is %d.", error_code);
    }
    return buffer;
}

static void sigint_handler(int signal);
static void print_session_stats(struct tftp_session_stats stats[static 1]);
static bool print_server_stats(struct tftp_server_stats stats[static 1]);

static struct tftp_server server;

int main(int argc, char *argv[static argc + 1]) {
    enum { OLD, NEW };
    struct sigaction action[2] = {};
    struct logger logger;
    struct cli_args args = {};
    if (!cli_args_parse(&args, argc, (const char **) argv)) {
        return EXIT_FAILURE;
    }
    if (!logger_init(&logger, logger_default_config)) {
        return EXIT_FAILURE;
    }
    logger.config.default_level = args.verbose_level;
    packet_loss_init(&logger, args.loss_probability, args.workers * args.max_worker_sessions);
    if (sigaction(SIGINT, nullptr, &action[OLD]) == -1) {
        logger_log_error(&logger, "Could not get the current SIGINT handler. %s", strerror_rbs(errno));
        return EXIT_FAILURE;
    }
    memcpy(&action[NEW], &action[OLD], sizeof *action);
    action[NEW].sa_handler = sigint_handler;
    const bool server_initialized = tftp_server_init(
        &server,
        (struct tftp_server_arguments){
            .ip = args.host,
            .port = args.port,
            .root = args.root,
            .retries = args.retries,
            .timeout_s = args.timeout_s,
            .workers = args.workers,
            .max_worker_sessions = args.max_worker_sessions,
            .is_adaptive_timeout_enabled = args.enable_adaptive_timeout,
            .is_write_request_enabled = args.enable_write_requests,
            .is_list_request_enabled = args.enable_list_requests,
            .server_stats_callback = print_server_stats,
            .session_stats_callback = print_session_stats,
            .stats_interval_seconds = datapoints_interval_seconds,
        },
        &logger);
    if (!server_initialized) {
        logger_log_error(&logger, "Could not initialize server.");
        goto fail;
    }
    if (sigaction(SIGINT, &action[NEW], nullptr) == -1
        || sigaction(SIGTERM, &action[NEW], nullptr) == -1) {
        logger_log_error(&logger, "Could not set the new SIGINT handler. %s", strerror_rbs(errno));
        goto fail;
    }
    if (!tftp_server_start(&server)) {
        logger_log_error(&logger, "Server encountered an error.");
        goto fail;
    }
    if (sigaction(SIGINT, &action[OLD], nullptr) == -1) {
        logger_log_error(&logger, "Could not restore the old SIGINT handler. %s", strerror_rbs(errno));
        goto fail;
    }
    tftp_server_destroy(&server);
    cli_args_destroy(&args);
    return EXIT_SUCCESS;
fail:
    cli_args_destroy(&args);
    return EXIT_FAILURE;
}

static void sigint_handler([[maybe_unused]] int signal) {
    tftp_server_stop(&server);
}

static void print_session_stats(struct tftp_session_stats stats[static 1]) {
    const int time_spent = (int) (difftime(time(nullptr), stats->start_time) * 1000);
    logger_log_info(stats->logger, "Stats: for %s requesting %s [%s]", stats->peer_addr, stats->file_path, stats->mode);
    if (stats->error.error_occurred) {
        logger_log_warn(stats->logger, "\tError: %s", stats->error.error_message);
    }
    logger_log_info(stats->logger, "\tTime spent: %dms", time_spent);
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

static bool print_server_stats(struct tftp_server_stats stats[static 1]) {
    if (mtx_lock(&stats->mtx) == thrd_error) {
        logger_log_error(stats->logger, "Could not lock server stats mutex. %s", strerror_rbs(errno));
        return false;
    }
    auto const counters = stats->counters;
    stats->counters = (struct tftp_server_stats_counters) {};
    if (mtx_unlock(&stats->mtx) == thrd_error) {
        logger_log_error(stats->logger, "Could not unlock server stats mutex. %s", strerror_rbs(errno));
        return false;
    }
    logger_log_info(stats->logger, "Server stats - every %d seconds", stats->interval);
    logger_log_info(stats->logger, "Number of spawned TFTP sessions in stats time frame : %lu", counters.sessions_count);
    return true;
}
