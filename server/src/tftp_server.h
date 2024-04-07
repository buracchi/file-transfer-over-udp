#pragma once

#include <netdb.h>
#include <signal.h>
#include <stdint.h>
#include <sys/socket.h>

#include <liburing.h>

#include "logger.h"
#include "tftp_server_stats.h"
#include "session_stats.h"
#include "uring_thread_pool.h"

//How many seconds to aggregate before sampling datapoints
constexpr int datapoints_interval_seconds = 60;

struct tftp_server {
    struct logger *logger;
    volatile sig_atomic_t should_stop; // volatile sig_atomic_t is used instead of atomic_bool for N3220 5.1.2.4/5 since it's implementation-defined whether the type is lock-free
    struct uring_thread_pool *thread_pool;
    struct io_uring ring;
    bool is_on_data_available_job_pending;
    bool is_timeout_job_pending;
    struct uring_job data_available_job;
    struct uring_job remove_data_available_job;
    struct uring_job timeout_job;
    struct uring_job remove_timeout_job;
    struct {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addrlen;
        struct iovec iovec[1];
        struct msghdr msghdr;
        uint8_t buffer[tftp_request_packet_max_size];
    } request_context;
    struct sockaddr_storage addr_storage;
    struct addrinfo addrinfo;
    int listener;
    const char *root;
    uint8_t retries;
    uint8_t timeout;
    struct tftp_server_stats stats;
    struct __kernel_timespec stats_timeout_timespec;
    void (*server_stats_callback)(struct tftp_server_stats *);
    void (*handler_stats_callback)(struct tftp_session_stats *);
    // Opt-in features
    bool is_netascii_enabled;
    bool is_write_request_enabled;
    bool is_list_request_enabled;
};

struct tftp_server_arguments {
    char *ip;
    char *port;
    const char *root;
    uint8_t retries;
    uint8_t timeout_s;
    uint32_t workers;
    uint16_t max_worker_sessions;
    void (*server_stats_callback)(struct tftp_server_stats *);
    void (*handler_stats_callback)(struct tftp_session_stats *);
    int stats_interval_seconds;
};

bool tftp_server_init(struct tftp_server server[static 1], struct tftp_server_arguments args, struct logger logger[static 1]);

bool tftp_server_start(struct tftp_server server[static 1]);

static inline void tftp_server_stop(struct tftp_server server[static 1]) {
    server->should_stop = true;
}

void tftp_server_destroy(struct tftp_server server[static 1]);
