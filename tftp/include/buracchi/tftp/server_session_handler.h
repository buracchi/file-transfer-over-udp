#ifndef TFTP_HANDLER_H
#define TFTP_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <netdb.h>

#include <logger.h>

#include <buracchi/tftp/dispatcher.h>
#include <buracchi/tftp/server_session_stats.h>

enum tftp_handler_action {
    TFTP_HANDLER_ACTION_NONE,
    TFTP_HANDLER_ACTION_SEND_OACK,
    TFTP_HANDLER_ACTION_SEND_DATA,
    TFTP_HANDLER_ACTION_SEND_ERROR,
    TFTP_HANDLER_ACTION_FETCH_NEXT_BLOCK,
    TFTP_HANDLER_ACTION_WAIT_NEW_DATA,
    TFTP_HANDLER_ACTION_CANCEL_TIMEOUT,
    TFTP_HANDLER_ACTION_UNEXPECTED,
};

struct tftp_handler_listener {
    int file_descriptor;
    struct sockaddr_storage addr_storage;
    struct addrinfo addrinfo;
    struct sockaddr_storage peer_addr_storage;
    socklen_t peer_addrlen;
    struct sockaddr_storage recv_peer_addr_storage;
    socklen_t recv_peer_addrlen;
    uint8_t recv_buffer[tftp_default_blksize];
    // TODO: As of Linux kernel version 6.10.4 IO_URING_OP_RECVFROM is not implemented, this is a workaround.
    //  Remove this fields and use io_uring_prep_recvfrom when available.
    struct msghdr msghdr;
    struct iovec iovec[1];
};

struct tftp_handler {
    struct logger *logger;
    bool should_stop;
    struct tftp_handler_listener listener;

    struct tftp_error_packet *error_packet;
    struct tftp_oack_packet *oack_packet;

    uint8_t retries;
    uint8_t timeout;
    uint16_t block_size;
    uint16_t window_size;

    uint8_t retransmits;
    int global_retransmits;

    struct tftp_data_packet *data_packets;  // array to window size elements of sizeof((struct tftp_data_packet)) + block_size bytes
    uint16_t window_begin;
    uint16_t next_data_packet_to_make;
    uint16_t next_data_packet_to_send;
    size_t last_block_size;   // last data packet may have less than block_size used bytes
    bool waiting_last_ack;
    const char *path;
    enum tftp_mode mode;
    
    int file_descriptor;
    struct iovec *file_iovec;
    bool incomplete_read;
    int netascii_buffer; // buffer for control character that won't fit in the current packet and must be split

    struct tftp_session_stats stats;
    bool valid_options_required;
    bool options_acknowledged;
    struct tftp_option options[TFTP_OPTION_TOTAL_OPTIONS];
    char options_storage[tftp_request_packet_max_size - sizeof(enum tftp_opcode)];
    char *options_str;
    size_t options_str_size;
};

bool tftp_handler_init(struct tftp_handler handler[static 1],
                       const struct addrinfo server_addr[static 1],
                       struct sockaddr_storage peer,
                       socklen_t peer_addrlen,
                       bool peer_demand_ipv4,
                       const char* root,
                       size_t n,
                       const char options[static n],
                       uint8_t retries,
                       uint8_t timeout,
                       void (*stats_callback)(struct tftp_session_stats *),
                       struct logger logger[static 1]);

enum tftp_handler_action tftp_handler_start(struct tftp_handler handler[static 1]);

void tftp_handler_resume(struct tftp_handler handler[static 1], struct dispatcher_event event[static 1]);

void tftp_handler_destroy(struct tftp_handler handler[static 1]);

enum tftp_handler_action tftp_handler_on_error_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_oack_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_data_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_next_block_ready(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_new_data(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_timeout(struct tftp_handler handler[static 1], struct dispatcher_event *event);

enum tftp_handler_action tftp_handler_on_timeout_removed(struct tftp_handler handler[static 1], struct dispatcher_event *event);

void listener_setup(struct tftp_handler handler[static 1]);

void next_block_setup(struct tftp_handler handler[static 1]);

ssize_t error_packet_init(struct tftp_handler handler[static 1]);

ssize_t oack_packet_init(struct tftp_handler handler[static 1]);

#endif // TFTP_HANDLER_H
