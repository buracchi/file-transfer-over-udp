#include <buracchi/tftp/server_session_handler.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <buracchi/tftp/dispatcher.h>
#include <buracchi/tftp/server_session_stats.h>

#include <logger.h>
#include <tftp.h>

#include "../utils/inet.h"

static void next_block_netascii(struct tftp_handler handler[static 1]);

static bool listener_init(struct tftp_handler handler[static 1], bool peer_demand_ipv4);
static bool required_file_setup(struct tftp_handler handler[static 1], const char *root);
static bool parse_options(struct tftp_handler handler[static 1]);
static bool parse_mode(struct tftp_handler handler[static 1], const char mode[static 1]);

static bool end_of_file(struct tftp_handler handler[static 1]);
static bool file_size_octet(int fd, size_t size[static 1]);
static bool file_size_netascii(int fd, size_t size[static 1]);

/** if n > tftp_request_packet_max_size - sizeof(enum tftp_opcode) behaviour is undefined **/
// TODO: Reduce cognitive complexity
bool tftp_handler_init(struct tftp_handler handler[static 1],
                       const struct addrinfo server_addr[static 1],
                       struct sockaddr_storage peer,
                       socklen_t peer_addrlen,
                       bool peer_demand_ipv4,
                       const char *root,
                       size_t n,
                       const char options[static n],
                       uint8_t retries,
                       uint8_t timeout,
                       void (*stats_callback)(struct tftp_session_stats *),
                       struct logger logger[static 1]) {
    *handler = (struct tftp_handler) {
        .logger = logger,
        .listener = {
            .file_descriptor = -1,
            .peer_addr_storage = peer,
            .peer_addrlen = peer_addrlen,
        },
        .error_packet = nullptr,
        .oack_packet = nullptr,
        .data_packets = nullptr,
        .retries = retries,
        .timeout = timeout,
        .block_size = tftp_default_blksize,
        .window_size = 1,
        .file_descriptor = -1,
        .file_iovec = nullptr,
        .next_data_packet_to_send = 1,
        .next_data_packet_to_make = 1,
        .netascii_buffer = -1,
    };

    {   // TODO: Remove block statement when io_uring_prep_recvfrom will be available.
        handler->listener.msghdr.msg_iov = handler->listener.iovec;
        handler->listener.msghdr.msg_iovlen = 1;
        handler->listener.msghdr.msg_control = nullptr;
        handler->listener.msghdr.msg_controllen = 0;
        handler->listener.msghdr.msg_flags = 0;
    }
    
    memcpy(&handler->listener.addr_storage, server_addr->ai_addr, server_addr->ai_addrlen);
    memcpy(&handler->listener.addrinfo, server_addr, sizeof *server_addr);
    handler->listener.addrinfo.ai_addr = (struct sockaddr *) &handler->listener.addr_storage;
    
    memcpy(handler->options_storage, options, n);
    handler->path = &handler->options_storage[0];
    const char *mode = &handler->options_storage[strlen(handler->path) + 1];
    handler->options_str_size = n - (strlen(handler->path) + 1 + strlen(mode) + 1);
    handler->options_str = handler->options_str_size == 0 ? nullptr : &handler->options_storage[n - handler->options_str_size];
    
    if (!listener_init(handler, peer_demand_ipv4)) {
        logger_log_error(logger, "Could not initialize session listener. Ignoring request.");
        return false;
    }
    handler->stats = tftp_session_stat_init(handler->listener.addrinfo.ai_addr, (const struct sockaddr *) &peer, handler->path, stats_callback, logger);
    if (!parse_mode(handler, mode)) {
        return true;
    }
    if (!required_file_setup(handler, root)) {
        return true;
    }
    if (!parse_options(handler)) {
        return true;
    }
    handler->data_packets = malloc(handler->window_size * (sizeof *handler->data_packets + handler->block_size));
    if (handler->data_packets == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    handler->file_iovec = malloc(handler->window_size * sizeof *handler->file_iovec);
    if (handler->file_iovec == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    return true;
}

enum tftp_handler_action tftp_handler_start(struct tftp_handler handler[static 1]) {
    if (handler->stats.error.error_occurred) {
        return TFTP_HANDLER_ACTION_SEND_ERROR;
    }
    if (handler->valid_options_required) {
        return TFTP_HANDLER_ACTION_SEND_OACK;
    }
    handler->window_begin = 1;
    if (handler->mode == TFTP_MODE_OCTET) {
        return TFTP_HANDLER_ACTION_FETCH_NEXT_BLOCK;
    }
    next_block_netascii(handler);
    return TFTP_HANDLER_ACTION_SEND_DATA;
}

void tftp_handler_destroy(struct tftp_handler handler[static 1]) {
    handler->stats.retransmits = handler->global_retransmits;
    handler->stats.callback(&handler->stats);
    logger_log_debug(handler->logger, "Closing response data object.");
    close(handler->file_descriptor);
    logger_log_debug(handler->logger, "Closing socket.");
    close(handler->listener.file_descriptor);
    logger_log_debug(handler->logger, "Session closed.");
    free(handler->data_packets);
    free(handler->file_iovec);
}

enum tftp_handler_action tftp_handler_on_error_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    free(handler->error_packet);
    if (!event->is_success) {
        logger_log_error(handler->logger, "Error while sending error: %s", strerror(event->error_number));
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    logger_log_trace(handler->logger, "Sent ERROR <message=%s> to %s:%d", handler->stats.error.error_message, handler->stats.peer_addr, handler->stats.peer_port);
    handler->should_stop = true;
    return TFTP_HANDLER_ACTION_CANCEL_TIMEOUT;
}

enum tftp_handler_action tftp_handler_on_oack_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (!event->is_success) {
        logger_log_error(handler->logger, "Error while sending OACK: %s", strerror(event->error_number));
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    logger_log_trace(handler->logger, "Sent OACK %s to %s:%d", handler->stats.options_acked, handler->stats.peer_addr, handler->stats.peer_port);
    handler->stats.packets_sent += 1;
    return TFTP_HANDLER_ACTION_WAIT_NEW_DATA;
}

enum tftp_handler_action tftp_handler_on_data_sent(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (!event->is_success) {
        logger_log_error(handler->logger, "Error while sending data: %s", strerror(event->error_number));
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    logger_log_trace(handler->logger, "Sent DATA <block=%d, size=%zu bytes> to %s:%d", handler->next_data_packet_to_send, handler->block_size, handler->stats.peer_addr, handler->stats.peer_port);
    handler->stats.packets_sent += 1;
    handler->next_data_packet_to_send += 1;
    bool is_last_packet = (handler->next_data_packet_to_send == handler->next_data_packet_to_make) &&
                          (handler->last_block_size < handler->block_size);
    if (is_last_packet) {
        handler->waiting_last_ack = true;
    }
    bool is_packet_last_in_window = handler->next_data_packet_to_send - handler->window_size == handler->window_begin;
    if (is_last_packet || is_packet_last_in_window) {
        return TFTP_HANDLER_ACTION_WAIT_NEW_DATA;
    }
    return TFTP_HANDLER_ACTION_SEND_DATA;
}

enum tftp_handler_action tftp_handler_on_next_block_ready(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (!event->is_success) {
        logger_log_warn(handler->logger, "Error while reading from source: %s", strerror(event->error_number));
        handler->stats.error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_NOT_DEFINED,
                .error_message = "Error while reading from source"
        };
        return TFTP_HANDLER_ACTION_SEND_ERROR;
    }
    size_t bytes_read = event->result;
    size_t writable_blocks = handler->window_size - (handler->next_data_packet_to_make - handler->window_begin);
    size_t whole_blocks_read = bytes_read / handler->block_size;
    if (handler->last_block_size == 0 && whole_blocks_read == writable_blocks) {
        handler->last_block_size = handler->block_size;
    }
    else {
        handler->last_block_size += bytes_read - (handler->block_size * whole_blocks_read);
        if (handler->last_block_size > handler->block_size) {
            handler->last_block_size %= handler->block_size;
        }
    }
    size_t blocks_to_make = whole_blocks_read;
    if (whole_blocks_read != writable_blocks && end_of_file(handler)) {
        blocks_to_make++;   // add the final block
    }
    for (size_t i = 0; i < blocks_to_make; i++) {
        size_t next_data_packet_index = (handler->next_data_packet_to_make - 1) % handler->window_size;
        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
        struct tftp_data_packet *packet = (struct tftp_data_packet *) ((uint8_t *) handler->data_packets + offset);
        tftp_data_packet_init(packet, handler->next_data_packet_to_make);
        handler->next_data_packet_to_make += 1;
    }
    if (whole_blocks_read != writable_blocks && !end_of_file(handler)) {
        handler->incomplete_read = true;
        return TFTP_HANDLER_ACTION_FETCH_NEXT_BLOCK;
    }
    return TFTP_HANDLER_ACTION_SEND_DATA;
}

enum tftp_handler_action tftp_handler_on_new_data(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (!event->is_success && event->error_number == ECANCELED) {
        return TFTP_HANDLER_ACTION_NONE;
    }
    if (!event->is_success) {
        logger_log_error(handler->logger, "Error while receiving data: %s", strerror(event->error_number));
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    // TODO: ret must be at least 4 bytes
    bool is_address_family_valid = false;
    switch (handler->listener.recv_peer_addr_storage.ss_family) {
        case AF_INET:
            handler->listener.recv_peer_addrlen = sizeof(struct sockaddr_in);
            is_address_family_valid = true;
            break;
        case AF_INET6:
            handler->listener.recv_peer_addrlen = sizeof(struct sockaddr_in6);
            is_address_family_valid = true;
            break;
        default:
            logger_log_warn(handler->logger, "Unknown address family: %d. Ignoring packet.", handler->listener.recv_peer_addr_storage.ss_family);
            break;
    }
    if (!is_address_family_valid) {
        return TFTP_HANDLER_ACTION_WAIT_NEW_DATA;
    }
    struct sockaddr *recv_peer_addr = (struct sockaddr *) &handler->listener.recv_peer_addr_storage;
    socklen_t recv_peer_addrlen = handler->listener.recv_peer_addrlen;
    if (recv_peer_addrlen != handler->listener.peer_addrlen ||
        memcmp(&handler->listener.peer_addr_storage, recv_peer_addr, recv_peer_addrlen) != 0) {
        char peer_addr_str[INET6_ADDRSTRLEN];
        uint16_t peer_port;
        inet_ntop_address(recv_peer_addr, peer_addr_str, &peer_port);
        logger_log_warn(handler->logger, "Unexpected peer: '%s:%d', expected peer: '%s:%d'.", peer_addr_str, peer_port, handler->stats.peer_addr, handler->stats.peer_port);
        handler->should_stop = true;
        return TFTP_HANDLER_ACTION_CANCEL_TIMEOUT;
    }
    enum tftp_opcode opcode = ntohs(*(uint16_t *) handler->listener.recv_buffer);
    uint16_t block_number = ntohs(*(uint16_t *) &handler->listener.recv_buffer[2]);
    switch (opcode) {
        case TFTP_OPCODE_ERROR:
            struct tftp_error_packet *error_packet = (struct tftp_error_packet *) handler->listener.recv_buffer;
            uint16_t error_code = ntohs(error_packet->error_code);
            char *error_message;
            const char *end_ptr = memchr(error_packet->error_message, '\0', event->result - sizeof *error_packet);
            if (end_ptr == nullptr) {
                error_message = "Error message received was invalid (not null terminated).";
            } else {
                size_t error_message_len = end_ptr - (char *) error_packet->error_message;
                error_message = malloc(error_message_len + 1);
                if (error_message == nullptr) {
                    logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
                    return TFTP_HANDLER_ACTION_UNEXPECTED;
                }
                strcpy(error_message, (const char *) error_packet->error_message);
            }
            handler->stats.error = (struct tftp_session_stats_error) {
                    .error_occurred = true,
                    .error_number = error_code,
                    .error_message = error_message,
            };
            logger_log_warn(handler->logger, "Error reported from client: %s",
                            handler->stats.error.error_message);
            return TFTP_HANDLER_ACTION_SEND_ERROR;
        case TFTP_OPCODE_ACK:
            logger_log_trace(handler->logger, "Received ACK <block=%d> from %s:%d", block_number,
                             handler->stats.peer_addr, handler->stats.peer_port);
            size_t payload_size = 0;
            uint16_t last_packet_sent = handler->next_data_packet_to_send - 1;
            if (block_number < handler->window_begin || block_number > last_packet_sent) {
                return TFTP_HANDLER_ACTION_WAIT_NEW_DATA; // ignore unexpected ACK
            }
            if (handler->valid_options_required && block_number == 0) {
                handler->options_acknowledged = true;
            } else {
                payload_size = handler->block_size * (block_number - handler->window_begin) +
                               (block_number == last_packet_sent ? handler->last_block_size
                                                                 : handler->block_size);
            }
            handler->window_begin = block_number + 1;
            handler->stats.packets_acked += 1;
            handler->stats.bytes_sent += payload_size;
            handler->retransmits = 0;
            if (handler->waiting_last_ack) {
                if (block_number == last_packet_sent) {
                    handler->should_stop = true;
                    return TFTP_HANDLER_ACTION_CANCEL_TIMEOUT;
                }
                return TFTP_HANDLER_ACTION_WAIT_NEW_DATA;
            }
            return TFTP_HANDLER_ACTION_FETCH_NEXT_BLOCK;
        default:
            logger_log_error(handler->logger, "Expected an ACK opcode from %s, got: %hu.",
                             handler->stats.peer_addr, opcode);
            handler->stats.error = (struct tftp_session_stats_error) {
                    .error_occurred = true,
                    .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
                    .error_message = "I only do reads for now...",
            };
            return TFTP_HANDLER_ACTION_SEND_ERROR;
    }
}

enum tftp_handler_action tftp_handler_on_timeout(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (!event->is_success && (event->error_number == ENOENT || event->error_number == ECANCELED)) {
        return TFTP_HANDLER_ACTION_NONE;
    }
    else if (!event->is_success && event->error_number != ETIME) {
        logger_log_error(handler->logger, "Error while handling timeout: %s", strerror(event->error_number));
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    if (handler->retransmits < handler->retries) {
        logger_log_debug(handler->logger, "ACK timeout for peer %s:%d. Retransmission no %d.", handler->stats.peer_addr, handler->stats.peer_port, handler->retransmits + 1);
        handler->retransmits += 1;
        handler->global_retransmits += 1;
        if (handler->valid_options_required && !handler->options_acknowledged) {
            // TODO: Use a persistent tftp_oack_packet instead of a pointer to a malloc.
            ssize_t packet_len = oack_packet_init(handler);
            if (packet_len == -1) {
                logger_log_error(handler->logger, "Could not initialize OACK packet.");
                return false;
            }
            return TFTP_HANDLER_ACTION_SEND_OACK;
        }
        handler->next_data_packet_to_send = handler->window_begin;
        return TFTP_HANDLER_ACTION_SEND_DATA;
    }
    static const char format[] = "timeout after %d retransmits.";
    static const char format_miss_last_ack[] = "timeout after %d retransmits. Missed last ack.";
    char *error_msg = malloc(sizeof(format_miss_last_ack) + 3 * sizeof(handler->retransmits) + 2);
    if (error_msg == nullptr) {
        logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    sprintf(error_msg, handler->waiting_last_ack ? format : format_miss_last_ack, handler->retransmits);
    handler->stats.error.error_occurred = true;
    handler->stats.error.error_number = TFTP_ERROR_NOT_DEFINED;
    handler->stats.error.error_message = error_msg;
    ssize_t error_packet_len = error_packet_init(handler);
    if (error_packet_len == -1) {
        logger_log_error(handler->logger, "Could not initialize error packet.");
        return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
    return TFTP_HANDLER_ACTION_SEND_ERROR;
}

enum tftp_handler_action tftp_handler_on_timeout_removed(struct tftp_handler handler[static 1], struct dispatcher_event *event) {
    if (event->is_success) {
        return TFTP_HANDLER_ACTION_NONE;
    }
    switch (event->error_number) {
        case 0:
            [[fallthrough]];
        case ENOENT:
            return TFTP_HANDLER_ACTION_NONE;
        default:
            logger_log_error(handler->logger, "Error while removing timeout: %s", strerror(event->error_number));
            return TFTP_HANDLER_ACTION_UNEXPECTED;
    }
}

void listener_setup(struct tftp_handler handler[static 1]) {
    handler->listener.recv_peer_addrlen = sizeof handler->listener.recv_peer_addr_storage;
    handler->listener.iovec[0].iov_base = handler->listener.recv_buffer;
    handler->listener.iovec[0].iov_len = sizeof handler->listener.recv_buffer;
    handler->listener.msghdr.msg_name = &handler->listener.recv_peer_addr_storage;
    handler->listener.msghdr.msg_namelen = handler->listener.recv_peer_addrlen;
}

void next_block_setup(struct tftp_handler handler[static 1]) {
    size_t available_packets = handler->window_size - (handler->next_data_packet_to_make - handler->window_begin);
    for (size_t i = 0; i < available_packets; i++) {
        size_t next_data_packet_index = (handler->next_data_packet_to_make - 1 + i) % handler->window_size;
        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
        struct tftp_data_packet *packet = (void *) ((uint8_t *) handler->data_packets + offset);
        handler->file_iovec[i].iov_base = packet->data;
        handler->file_iovec[i].iov_len = handler->block_size;
    }
    // handle partial reads
    handler->last_block_size = handler->incomplete_read ? handler->last_block_size : 0;
    handler->incomplete_read = false;
    handler->file_iovec[0].iov_base = &((uint8_t *) handler->file_iovec[0].iov_base)[handler->last_block_size];
    handler->file_iovec[0].iov_len -= handler->last_block_size;
}

ssize_t error_packet_init(struct tftp_handler handler[static 1]) {
    ssize_t packet_len = sizeof(struct tftp_error_packet) + strlen(handler->stats.error.error_message) + 1;
    handler->error_packet = malloc(packet_len);
    if (handler->error_packet == nullptr) {
        logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
        return -1;
    }
    memcpy(handler->error_packet,
           &(struct tftp_error_packet) {
                   .opcode = htons(TFTP_OPCODE_ERROR),
                   .error_code = htons(handler->stats.error.error_number)
           },
           sizeof *handler->error_packet);
    strcpy((char *) handler->error_packet->error_message, handler->stats.error.error_message);
    return packet_len;
}

ssize_t oack_packet_init(struct tftp_handler handler[static 1]) {
    size_t options_values_length = 0;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (handler->options[option].is_active) {
            options_values_length += strlen(tftp_option_recognized_string[option]) + 1;
            options_values_length += strlen(handler->options[option].value) + 1;
        }
    }
    ssize_t packet_len = sizeof(struct tftp_oack_packet) + options_values_length;
    handler->oack_packet = malloc(packet_len);
    if (handler->oack_packet == nullptr) {
        logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
        return -1;
    }
    handler->oack_packet->opcode = htons(TFTP_OPCODE_OACK);
    unsigned char *ptr = handler->oack_packet->options_values;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (handler->options[option].is_active) {
            size_t len = strlen(tftp_option_recognized_string[option]) + 1;
            ptr = (unsigned char *) memcpy(ptr, tftp_option_recognized_string[option], len) + len;
            len = strlen(handler->options[option].value) + 1;
            ptr = (unsigned char *) memcpy(ptr, handler->options[option].value, len) + len;
        }
    }
    return packet_len;
}

/**
 * This is comically bad.
 * Never opt in NetASCII since I don't plan to improve this in the near future.
 */
static void next_block_netascii(struct tftp_handler handler[static 1]) {
    size_t available_packets = handler->window_size - (handler->next_data_packet_to_make - handler->window_begin);
    for (size_t i = 0; i < available_packets; i++) {
        size_t next_data_packet_index = (handler->next_data_packet_to_make - 1) % handler->window_size;
        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
        struct tftp_data_packet *packet = (void *) ((uint8_t *) handler->data_packets + offset);
        uint8_t *block = packet->data;
        while (block - packet->data < handler->block_size) {
            if (handler->netascii_buffer != -1) {
                *block++ = handler->netascii_buffer;
                handler->netascii_buffer = -1;
            }
            ssize_t byte_read = read(handler->file_descriptor, block, 1);
            if (byte_read == 0) {
                break;
            }
            int c = *block++;
            if (c == '\n' || c == '\r') {
                if (block - packet->data == handler->block_size) {
                    handler->netascii_buffer = c;
                }
                else {
                    *(block++) = (c == '\n') ? '\r' : '\0';
                }
            }
        }
        tftp_data_packet_init(packet, handler->next_data_packet_to_make);
        handler->next_data_packet_to_make += 1;
        handler->last_block_size = block - packet->data;
    }
}

static bool parse_mode(struct tftp_handler handler[static 1], const char mode[static 1]) {
    if (strcasecmp(mode, "octet") == 0) {
        handler->mode = TFTP_MODE_OCTET;
    }
    else if (strcasecmp(mode, "netascii") == 0) {
        handler->mode = TFTP_MODE_NETASCII;
    }
    else {
        handler->mode = TFTP_MODE_INVALID;
        const char format[] = "Unknown mode: '%s'.";
        char *message = malloc(sizeof format + strlen(mode));
        if (message == nullptr) {
            // TODO: handle this better
            logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        sprintf(message, format, mode);
        handler->stats.error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
                .error_message = message,
        };
    }
    handler->stats.mode = tftp_mode_to_string(handler->mode);
    return (handler->mode != TFTP_MODE_INVALID);
}

static bool parse_options(struct tftp_handler handler[static 1]) {
    if (handler->options_str == nullptr) {
        logger_log_info(handler->logger, "No options requested from peer %s:%d.", handler->stats.peer_addr, handler->stats.peer_port);
        return true;
    }
    tftp_format_option_strings(handler->options_str_size, handler->options_str, handler->stats.options_in);
    logger_log_info(handler->logger, "Options requested from peer %s:%d are [%s]", handler->stats.peer_addr, handler->stats.peer_port, handler->stats.options_in);
    tftp_parse_options(handler->options, handler->options_str_size, handler->options_str);
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (handler->options[o].is_active) {
            handler->valid_options_required = true;
            switch (o) {
                case TFTP_OPTION_BLKSIZE:
                    handler->block_size = strtoul(handler->options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TIMEOUT:
                    handler->timeout = strtoul(handler->options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TSIZE:
                    size_t size;
                    bool (*file_size)(int, size_t *) = (handler->mode == TFTP_MODE_OCTET) ?
                                                       file_size_octet :
                                                       file_size_netascii;
                    if (!file_size(handler->file_descriptor, &size)) {
                        handler->stats.error = (struct tftp_session_stats_error) {
                            .error_occurred = true,
                            .error_number = TFTP_ERROR_INVALID_OPTIONS,
                            .error_message = "File does not support being queried for file size."
                        };
                        return false;
                    }
                    sprintf((char *) handler->options[o].value, "%zu", size);
                    break;
                case TFTP_OPTION_WINDOWSIZE:
                    handler->window_size = strtoul(handler->options[o].value, nullptr, 10);
                    break;
                default: // unreachable
                    break;
            }
        }
    }
    tftp_format_options(handler->options, handler->stats.options_acked);
    logger_log_info(handler->logger, "Options to ack for peer %s:%d are %s", handler->stats.peer_addr, handler->stats.peer_port, handler->stats.options_acked);
    handler->stats.blksize = handler->block_size;
    handler->stats.window_size = handler->window_size;
    return true;
}

static bool listener_init(struct tftp_handler handler[static 1], bool peer_demand_ipv4) {
    char peer_addr[INET6_ADDRSTRLEN];
    uint16_t peer_port;
    if (peer_demand_ipv4) {
        logger_log_debug(handler->logger, "Peer request an IPV4 response, using an IPV4 connection to support an IPV6 unaware client.");
        struct sockaddr_storage ipv4_serv_addr = {};
        struct sockaddr_storage ipv4_peer_addr = {};
        if (inet_add_in6_to_in((const struct sockaddr_in6 *) &handler->listener.addr_storage, (struct sockaddr_in *) &ipv4_serv_addr) < 0) {
            logger_log_error(handler->logger, "Could not translate server address to an IPV4 address.");
            return false;
        }
        memcpy(&handler->listener.addr_storage, &ipv4_serv_addr, sizeof ipv4_serv_addr);
        handler->listener.addrinfo.ai_addrlen = sizeof(struct sockaddr_in);
        handler->listener.addrinfo.ai_family = AF_INET;
        if (inet_add_in6_to_in((const struct sockaddr_in6 *) &handler->listener.peer_addr_storage, (struct sockaddr_in *) &ipv4_peer_addr) < 0) {
            logger_log_error(handler->logger, "Could not translate peer address to an IPV4 address.");
            return false;
        }
        memcpy(&handler->listener.peer_addr_storage, &ipv4_peer_addr, sizeof ipv4_peer_addr);
        handler->listener.peer_addrlen = sizeof(struct sockaddr_in);
    }
    else {
        handler->listener.peer_addrlen = sizeof(struct sockaddr_in6);
    }
    if (inet_ntop_address((const struct sockaddr *) &handler->listener.peer_addr_storage, peer_addr, &peer_port) == nullptr) {
        logger_log_error(handler->logger, "Could not translate peer address to a string representation.");
    }
    else {
        logger_log_info(handler->logger, "New incoming connection from peer '%s:%d' asking for path '%s'", peer_addr, peer_port, handler->path);
    }
    handler->listener.file_descriptor = socket(handler->listener.addrinfo.ai_family, SOCK_DGRAM, 0);
    if (handler->listener.file_descriptor == -1) {
        logger_log_error(handler->logger, "Could not create socket: %s", strerror(errno));
        return false;
    }
    // use an available ephemeral port for binding
    if (peer_demand_ipv4) {
        ((struct sockaddr_in *) handler->listener.addrinfo.ai_addr)->sin_port = 0;
    }
    else {
        ((struct sockaddr_in6 *) handler->listener.addrinfo.ai_addr)->sin6_port = 0;
    }
    if (bind(handler->listener.file_descriptor, handler->listener.addrinfo.ai_addr, handler->listener.addrinfo.ai_addrlen) == -1) {
        logger_log_error(handler->logger, "Could not bind socket: %s", strerror(errno));
        close(handler->listener.file_descriptor);
        return false;
    }
    if (getsockname(handler->listener.file_descriptor, handler->listener.addrinfo.ai_addr, &handler->listener.addrinfo.ai_addrlen) == -1) {
        logger_log_error(handler->logger, "Could not get socket name: %s", strerror(errno));
        close(handler->listener.file_descriptor);
        return false;
    }
    return true;
}

static bool required_file_setup(struct tftp_handler handler[static 1], const char *root) {
    const char *path;
    if (root == nullptr) {
        path = handler->path;
    }
    else {
        bool is_root_slash_terminated = root[strlen(root) - 1] == '/';
        size_t padding = is_root_slash_terminated ? 0 : 1;
        char *full_path = malloc(strlen(root) + strlen(handler->path) + padding + 1);
        if (full_path == nullptr) {
            // TODO: handle this better
            logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        strcpy(full_path, root);
        if (!is_root_slash_terminated) {
            strcat(full_path, "/");
        }
        strcat(full_path, handler->path);
        path = full_path;
    }
    errno = 0;
    do {
        handler->file_descriptor = open(path, O_RDONLY);
    } while (handler->file_descriptor == -1 && errno == EINTR);
    if (root != nullptr) {
        free((void *) path);
    }
    if (handler->file_descriptor == -1) {
        enum tftp_error_code error_code;
        char *message;
        switch (errno) {
            case EACCES:
                error_code = TFTP_ERROR_ACCESS_VIOLATION;
                message = "Permission denied.";
                break;
            case ENOENT:
                error_code = TFTP_ERROR_FILE_NOT_FOUND;
                message = "No such file or directory.";
                break;
            default:
                error_code = TFTP_ERROR_NOT_DEFINED;
                message = strerror(errno);
        }
        handler->stats.error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = error_code,
            .error_message = message,
        };
        return false;
    }
    return true;
}

static bool end_of_file(struct tftp_handler handler[static 1]) {
    off_t cur_offset = lseek(handler->file_descriptor, 0, SEEK_CUR);
    bool is_file_seekable = (cur_offset != -1);
    if (!is_file_seekable) {
        return handler->last_block_size == 0;
    }
    off_t end_offset = lseek(handler->file_descriptor, 0, SEEK_END);
    lseek(handler->file_descriptor, cur_offset, SEEK_SET);
    return cur_offset == end_offset;
}

static bool file_size_octet(int fd, size_t size[static 1]) {
    struct stat file_stat;
    if (fd < 0) {
        return false;
    }
    if (fstat(fd, &file_stat) < 0) {
        return false;
    }
    switch (file_stat.st_mode & __S_IFMT) {
        case __S_IFBLK:
            uint64_t bytes;
            if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
                return false;
            }
            *size = bytes;
            return true;
        case __S_IFREG:
            *size = file_stat.st_size;
            return true;
        default:
            return false;
    }
}

static bool file_size_netascii(int fd, size_t size[static 1]) {
    size_t netascii_size = 0;
    size_t bytes_to_read;
    if (!file_size_octet(fd, &bytes_to_read)) {
        return false;
    }
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);
    if (buffer == nullptr) {
        return false;
    }
    while (bytes_to_read) {
        ssize_t bytes_read = read(fd, buffer, buffer_size);
        if (bytes_read == -1) {
            free(buffer);
            return false;
        }
        netascii_size += bytes_read;
        for (size_t i = 0; i < buffer_size; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                ++netascii_size;
            }
        }
        bytes_to_read -= bytes_read;
    }
    free(buffer);
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return false;
    }
    *size = netascii_size;
    return true;
}
