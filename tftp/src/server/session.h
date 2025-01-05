#ifndef TFTP_SESSION_H
#define TFTP_SESSION_H

#include <netdb.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <time.h>

#include <tftp.h>
#include <logger.h>

#include <buracchi/tftp/server_listener.h>
#include <buracchi/tftp/server_stats.h>
#include <buracchi/tftp/server_session_stats.h>

#include "adaptive_timeout.h"
#include "dispatcher.h"
#include "session_connection.h"
#include "session_handler.h"

struct tftp_server_info {
    struct addrinfo* server_addrinfo;
    const char *root;
    uint8_t retries;
    uint8_t timeout;
    bool is_adaptive_timeout_enabled;
    bool is_write_request_enabled;
    bool is_list_request_enabled;
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
    bool is_handler_instantiated;
    struct tftp_server_info *server_info;

    struct session_connection connection;
    struct tftp_session_stats stats;
    
    bool should_close;
    int coroutine_state;
    
    uint8_t pending_jobs;
    struct dispatcher_event event_start;
    struct dispatcher_event_timeout event_timeout;
    struct dispatcher_event event_cancel_timeout;
    struct dispatcher_event event_new_data;
    struct dispatcher_event event_next_block;
    struct dispatcher_event event_packet_sent;
    
    struct adaptive_timeout adaptive_timeout;
    struct timespec last_send_time;
    bool is_timer_active;
    
    bool is_first_send;
    bool is_waiting_last_ack;
    
    struct tftp_error_packet *error_packet;
    size_t error_packet_size;
    struct tftp_oack_packet *oack_packet;
    size_t oack_packet_size;
};

enum tftp_session_state {
    TFTP_SESSION_STATE_IDLE,
    TFTP_SESSION_STATE_ERROR,
    TFTP_SESSION_STATE_CLOSED,
};

void tftp_session_init(struct tftp_session session[static 1],
                       uint16_t session_id,
                       struct tftp_server_info server_info[static 1],
                       struct dispatcher dispatcher[static 1],
                       struct logger logger[static 1]);

enum tftp_session_state tftp_session_handle_event(struct tftp_session session[static 1], struct dispatcher_event event[static 1]);

#endif // TFTP_SESSION_H
