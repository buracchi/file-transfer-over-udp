#ifndef TFTP_SERVER_H
#define TFTP_SERVER_H

#include <signal.h>
#include <stdint.h>

#include <logger.h>

#include <buracchi/tftp/server_stats.h>
#include <buracchi/tftp/server_session_stats.h>
#include <buracchi/tftp/server_listener.h>

//How many seconds to aggregate before sampling datapoints
constexpr int datapoints_interval_seconds = 60;

struct tftp_server {
    struct logger *logger;
    volatile sig_atomic_t should_stop; // volatile sig_atomic_t is used instead of atomic_bool for N3220 5.1.2.4/5 since it's implementation-defined whether the type is lock-free
    struct tftp_server_worker_pool *worker_pool;
    struct tftp_server_listener listener;
    struct tftp_server_stats stats;
    
    void (*handler_stats_callback)(struct tftp_session_stats *);
    
    const char *root;
    uint8_t retries;
    uint8_t timeout;

    // Opt-in features
    bool is_adaptive_timeout_enabled;
    bool is_write_request_enabled;
    bool is_list_request_enabled;
};

struct tftp_server_arguments {
    const char *ip;
    const char *port;
    const char *root;
    uint8_t retries;
    uint8_t timeout_s;
    uint16_t workers;
    uint16_t max_worker_sessions;
    bool is_adaptive_timeout_enabled;
    bool is_write_request_enabled;
    bool is_list_request_enabled;
    bool (*server_stats_callback)(struct tftp_server_stats *);
    void (*handler_stats_callback)(struct tftp_session_stats *);
    int stats_interval_seconds;
};

bool tftp_server_init(struct tftp_server server[static 1], struct tftp_server_arguments args, struct logger logger[static 1]);

bool tftp_server_start(struct tftp_server server[static 1]);

static inline void tftp_server_stop(struct tftp_server server[static 1]) {
    server->should_stop = true;
}

void tftp_server_destroy(struct tftp_server server[static 1]);

#endif // TFTP_SERVER_H
