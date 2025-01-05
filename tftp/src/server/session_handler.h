#ifndef TFTP_HANDLER_H
#define TFTP_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <netdb.h>

#include <logger.h>

#include <buracchi/tftp/server_session_stats.h>

#include "dispatcher.h"

struct tftp_handler {
    struct logger *logger;

    uint8_t retries;
    uint8_t timeout;
    uint16_t block_size;
    uint16_t window_size;
    bool adaptive_timeout;

    uint8_t retransmits;
    int global_retransmits;

    struct tftp_data_packet *data_packets;  // array to window size elements of sizeof((struct tftp_data_packet)) + block_size bytes
    uint16_t window_begin;
    uint16_t next_data_packet_to_make;
    uint16_t next_data_packet_to_send;
    size_t last_block_size;   // last data packet may have less than block_size used bytes
    const char *path;
    enum tftp_mode mode;
    
    int file_descriptor;
    struct iovec *file_iovec;
    bool incomplete_read;
    int netascii_buffer; // buffer for control character that won't fit in the current packet and must be split

    struct tftp_session_stats *stats;
    
    bool valid_options_required;
    bool options_acknowledged;
    struct tftp_option options[TFTP_OPTION_TOTAL_OPTIONS];
    char options_storage[tftp_request_packet_max_size - sizeof(enum tftp_opcode)];
    char *options_str;
    size_t options_str_size;
};

bool tftp_handler_init(struct tftp_handler handler[static 1],
                       const char* root,
                       size_t n,
                       const char options[static n],
                       uint8_t retries,
                       uint8_t timeout,
                       bool is_adaptive_timeout_enabled,
                       struct tftp_session_stats stats[static 1],
                       struct logger logger[static 1]);

void tftp_handler_destroy(struct tftp_handler handler[static 1]);

void next_block_setup(struct tftp_handler handler[static 1]);

#endif // TFTP_HANDLER_H
