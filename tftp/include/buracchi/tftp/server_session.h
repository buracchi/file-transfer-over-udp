#ifndef TFTP_SESSION_H
#define TFTP_SESSION_H

#include <netdb.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/socket.h>

#include <tftp.h>
#include <logger.h>

#include <buracchi/tftp/dispatcher.h>
#include <buracchi/tftp/server_listener.h>
#include <buracchi/tftp/server_stats.h>
#include <buracchi/tftp/server_session_stats.h>
#include <buracchi/tftp/server_session_handler.h>

struct tftp_server_info {
    struct addrinfo* server_addrinfo;
    const char *root;
    uint8_t retries;
    uint8_t timeout;
    void (*handler_stats_callback)(struct tftp_session_stats *);
    struct tftp_server_stats *server_stats;
};

struct tftp_session {
    uint16_t session_id;
    volatile atomic_bool is_active;
    struct dispatcher *dispatcher;
    struct logger *logger;
    struct tftp_peer_message request_args;
    struct tftp_handler handler;
    struct tftp_server_info *server_info;

    uint8_t pending_jobs;
    struct dispatcher_event event_start;
    struct dispatcher_event event_end;
    struct dispatcher_event_timeout event_timeout;
    struct dispatcher_event event_cancel_timeout;
    struct dispatcher_event event_new_data;
    struct dispatcher_event event_next_block;
    struct dispatcher_event event_transmit_data;
    struct dispatcher_event event_transmit_oack;
    struct dispatcher_event event_transmit_error;
};

enum tftp_session_state {
    TFTP_SESSION_IDLE,
    TFTP_SESSION_ERROR,
    TFTP_SESSION_CLOSED,
};

void tftp_session_init(struct tftp_session session[static 1],
                       uint16_t session_id,
                       struct tftp_server_info server_info[static 1],
                       struct dispatcher dispatcher[static 1],
                       struct logger logger[static 1]);

enum tftp_session_state tftp_session_on_event(struct tftp_session session[static 1], struct dispatcher_event event[static 1]);

#endif // TFTP_SESSION_H
