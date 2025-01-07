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
#include "session_options.h"

enum session_request_type {
    SESSION_READ_REQUEST,
    SESSION_WRITE_REQUEST,
};

struct tftp_server_info {
    struct addrinfo* server_addrinfo;
    const char *root;
    uint8_t retries;
    uint8_t timeout;
    bool is_adaptive_timeout_enabled;
    bool is_write_request_enabled;
    bool is_list_request_enabled;
    void (*session_stats_callback)(struct tftp_session_stats *);
    struct tftp_server_stats *server_stats;
};

struct tftp_session {
    volatile atomic_bool is_active;
    struct dispatcher *dispatcher;
    struct logger *logger;
    struct tftp_peer_message request_args;
    struct tftp_server_info *server_info;
    bool is_timer_active;
    
    enum session_request_type request_type;

    int file_descriptor;
    
    const char *filename;
    enum tftp_mode mode;
    uint8_t retries;
    uint8_t timeout;
    uint16_t block_size;
    uint16_t window_size;
    bool is_adaptive_timeout_active;
    
    uint8_t current_retransmission;
    int total_retransmissions;
    
    uint16_t window_begin;
    uint16_t next_data_packet_to_send;
    uint16_t expected_sequence_number;
    
    int netascii_buffer; // buffer for control character that won't fit in the current packet and must be split
    int32_t last_packet;
    uint16_t last_block_size;   // last data packet may have less than block_size used bytes
    bool incomplete_read;
    
    struct session_connection connection;
    struct tftp_session_stats stats;
    
    struct session_options options;
    
    bool is_fetching_data;
    bool should_close;
    
    uint8_t pending_jobs;
    struct dispatcher_event event_start;
    struct dispatcher_event event_cancel_timeout;
    struct dispatcher_event_timeout event_timeout;
    struct dispatcher_event event_cancel_packet_received;
    struct dispatcher_event event_packet_received;
    struct dispatcher_event event_next_block;
    
    struct adaptive_timeout adaptive_timeout;
    
    struct tftp_error_packet *error_packet;
    size_t error_packet_size;
    struct tftp_oack_packet *oack_packet;
    size_t oack_packet_size;
    struct tftp_data_packet *data_packets;
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
