#include "tftp_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include <liburing.h>

#include "inet_utils.h"
#include "tftp.h"

static void resume_execution(struct tftp_handler handler[static 1]);

static void submit_transmit_oack_request(struct tftp_handler handler[static 1]);
static void on_transmit_oack(int32_t ret, void *args);

static void submit_on_next_block_request(struct tftp_handler handler[static 1]);
static void on_next_block(int32_t ret, void *args);

static void next_block_netascii(struct tftp_handler handler[static 1]);

static void submit_on_transmit_data_request(struct tftp_handler handler[static 1]);
static void on_transmit_data(int32_t ret, void *args);

static void submit_on_transmit_error_request(struct tftp_handler handler[static 1]);
static void on_transmit_error(int32_t ret, void *args);

static void submit_on_new_data_request(struct tftp_handler handler[static 1]);
static void on_new_data(int32_t ret, void *args);

static void submit_remove_timeout_request(struct tftp_handler handler[static 1]);
static void on_timeout(int32_t ret, void *args);
static void on_timeout_removed(int32_t ret, void *args);

static void close_handler(struct tftp_handler handler[static 1]);

static void handle_new_data(struct tftp_handler handler[static 1], size_t n, uint8_t data[static n]);
static bool listener_init(struct tftp_handler handler[static 1], bool peer_demand_ipv4);
static bool required_file_setup(struct tftp_handler handler[static 1], const char *root);
static void parse_options(struct tftp_handler handler[static 1]);

static bool end_of_file(struct tftp_handler handler[static 1]);
static bool file_size_octet(int fd, size_t size[static 1]);
static bool file_size_netascii(int fd, size_t size[static 1]);

/** if n > tftp_request_packet_max_size - sizeof(enum tftp_opcode) behaviour is undefined **/
// TODO: Reduce cognitive complexity
void tftp_handler_init(struct tftp_handler handler[static 1],
                       struct io_uring *ring,
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
                       struct logger logger[static 1],
                       void (*after_closed_callback)(struct tftp_handler handler[static 1])) {
    *handler = (struct tftp_handler) {
        .logger = logger,
        .ring = ring,
        .timeout_job = {
            .routine = on_timeout,
            .args = handler,
        },
        .remove_timeout_job = {
            .routine = on_timeout_removed,
            .args = handler,
        },
        .listener_job = {
            .routine = on_new_data,
            .args = handler,
        },
        .aio_job = {
            .routine = nullptr,
            .args = handler,
        },
        .state = TFTP_HANDLER_STATE_MAKE_DATA_PACKET,
        .listener = {
            .file_descriptor = -1,
            .peer_addr_storage = peer,
            .peer_addrlen = peer_addrlen,
        },
        .retries = retries,
        .timeout = timeout,
        .block_size = tftp_default_blksize,
        .window_size = 1,
        .stats_callback = stats_callback,
        .file_descriptor = -1,
        .next_data_packet_to_send = 1,
        .next_data_packet_to_make = 1,
        .netascii_buffer = -1,
        .after_closed_callback = after_closed_callback,
    };

    handler->timeout_timespec.tv_sec = timeout;

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
        return;
    }
    
    handler->stats = tftp_session_stat_init(handler->listener.addrinfo.ai_addr, (const struct sockaddr *) &peer, handler->path, logger);
    
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
        sprintf(message, format, mode);
        handler->stats.error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
            .error_message = message,
        };
        handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
        return;
    }
    handler->stats.mode = tftp_mode_to_string(handler->mode);
    
    if (!required_file_setup(handler, root)) {
        handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
        return;
    }
    parse_options(handler);
    if (handler->valid_options_required) {
        handler->state = TFTP_HANDLER_STATE_SEND_OACK_PACKET;
    }
    else {
        handler->window_begin = 1;
    }
    handler->data_packets = malloc(handler->window_size * (sizeof *handler->data_packets + handler->block_size));
    if (handler->data_packets == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        exit(1);
    }
    handler->file_iovec = malloc(handler->window_size * sizeof *handler->file_iovec);
    if (handler->file_iovec == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        exit(1);
    }
    memset(handler->data_packets, 0, handler->window_size * (sizeof *handler->data_packets + handler->block_size));
}

void tftp_handler_start(struct tftp_handler handler[static 1]) {
    resume_execution(handler);
}

static void resume_execution(struct tftp_handler handler[1]) {
    switch (handler->state) {
        case TFTP_HANDLER_STATE_SEND_ERROR_PACKET:
            submit_on_transmit_error_request(handler);
            break;
        case TFTP_HANDLER_STATE_SEND_OACK_PACKET:
            submit_transmit_oack_request(handler);
            break;
        case TFTP_HANDLER_STATE_MAKE_DATA_PACKET:
            if (handler->mode == TFTP_MODE_OCTET) {
                submit_on_next_block_request(handler);
            }
            else {
                next_block_netascii(handler);
            }
            break;
        case TFTP_HANDLER_STATE_SEND_DATA_PACKET:
            submit_on_transmit_data_request(handler);
            break;
        case TFTP_HANDLER_STATE_WAIT_ACK_PACKET:
            submit_on_new_data_request(handler);
            break;
        case TFTP_HANDLER_STATE_CLOSE:
            close_handler(handler);
            break;
        case TFTP_HANDLER_STATE_AFTER_CLOSED:
            if (handler->after_closed_callback != nullptr) {
                handler->after_closed_callback(handler);
            }
            break;
        default:
            break;
    }
}

// TODO: Refactory when io_uring_prep_recvfrom will be available.
static void submit_on_new_data_request(struct tftp_handler handler[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    handler->listener.recv_peer_addrlen = sizeof handler->listener.recv_peer_addr_storage;
    handler->listener.iovec[0].iov_base = handler->listener.recv_buffer;
    handler->listener.iovec[0].iov_len = sizeof handler->listener.recv_buffer;
    handler->listener.msghdr.msg_name = &handler->listener.recv_peer_addr_storage;
    handler->listener.msghdr.msg_namelen = handler->listener.recv_peer_addrlen;
    io_uring_prep_recvmsg(sqe, handler->listener.file_descriptor, &handler->listener.msghdr, 0);
    io_uring_sqe_set_data(sqe, &handler->listener_job);
    io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK);
    sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_link_timeout(sqe, &handler->timeout_timespec, 0);
    io_uring_sqe_set_data(sqe, &handler->timeout_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting receive new data request: %s", strerror(-ret));
        exit(1);
    }
    handler->is_timeout_job_pending = true;
}

static void on_new_data(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    if (-ret == ECANCELED) {
        return;
    }
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while receiving data: %s", strerror(-ret));
        exit(1);
    }
    // TODO: ret must be at least 4 bytes
    switch (handler->listener.recv_peer_addr_storage.ss_family) {
        case AF_INET:
            handler->listener.recv_peer_addrlen = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            handler->listener.recv_peer_addrlen = sizeof(struct sockaddr_in6);
            break;
        default:
            logger_log_warn(handler->logger, "Unknown address family: %d. Ignoring packet.", handler->listener.recv_peer_addr_storage.ss_family);
            resume_execution(handler);
            return;
    }
    handle_new_data(handler, ret, handler->listener.recv_buffer);
    resume_execution(handler);
}

static void handle_new_data(struct tftp_handler handler[static 1], size_t n, uint8_t data[static n]) {
    struct sockaddr *recv_peer_addr = (struct sockaddr *) &handler->listener.recv_peer_addr_storage;
    socklen_t recv_peer_addrlen = handler->listener.recv_peer_addrlen;
    if (recv_peer_addrlen != handler->listener.peer_addrlen ||
        memcmp(&handler->listener.peer_addr_storage, recv_peer_addr, recv_peer_addrlen) != 0) {
        char peer_addr_str[INET6_ADDRSTRLEN];
        uint16_t peer_port;
        inet_ntop_address(recv_peer_addr, peer_addr_str, &peer_port);
        logger_log_error(handler->logger, "Unexpected peer: '%s:%d', expected peer: '%s:%d'.", peer_addr_str, peer_port, handler->stats.peer_addr, handler->stats.peer_port);
        handler->state = TFTP_HANDLER_STATE_CLOSE;
        return;
    }
    enum tftp_opcode opcode = ntohs(*(uint16_t *) data);
    uint16_t block_number = ntohs(*(uint16_t *) &data[2]);
    switch (opcode) {
        case TFTP_OPCODE_ERROR:
            struct tftp_error_packet *error_packet = (struct tftp_error_packet *) data;
            uint16_t error_code = ntohs(error_packet->error_code);
            char *error_message;
            const char *end_ptr = memchr(error_packet->error_message, '\0', n - sizeof *error_packet);
            if (end_ptr == nullptr) {
                error_message = "Error message received was invalid (not null terminated).";
            }
            else {
                size_t error_message_len = end_ptr - (char *) error_packet->error_message;
                error_message = malloc(error_message_len + 1);
                strcpy(error_message, (const char *) error_packet->error_message);
            }
            handler->stats.error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = error_code,
                .error_message = error_message,
            };
            logger_log_warn(handler->logger, "Error reported from client: %s", handler->stats.error.error_message);
            handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
            return;
        case TFTP_OPCODE_ACK:
            size_t payload_size = 0;
            uint16_t last_packet_sent = handler->next_data_packet_to_send - 1;
            if (block_number < handler->window_begin || block_number > last_packet_sent) {
                handler->state = TFTP_HANDLER_STATE_WAIT_ACK_PACKET;
                return; // ignore unexpected ACK
            }
            if (handler->valid_options_required && block_number == 0) {
                handler->options_acknowledged = true;
            }
            else {
                payload_size = handler->block_size * (block_number - handler->window_begin) +
                               (block_number == last_packet_sent ? handler->last_block_size : handler->block_size);
            }
            handler->window_begin = block_number + 1;
            handler->stats.packets_acked += 1;
            handler->stats.bytes_sent += payload_size;
            handler->retransmits = 0;
            if (handler->waiting_last_ack) {
                handler->state = (block_number == last_packet_sent) ?
                                 TFTP_HANDLER_STATE_CLOSE :
                                 TFTP_HANDLER_STATE_WAIT_ACK_PACKET;
            }
            else {
                handler->state = TFTP_HANDLER_STATE_MAKE_DATA_PACKET;
            }
            return;
        default:
            logger_log_error(handler->logger, "Expected an ACK opcode from %s, got: %hu.", handler->stats.peer_addr, opcode);
            handler->stats.error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
                .error_message = "I only do reads for now...",
            };
            handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
            return;
    }
}

static void submit_remove_timeout_request(struct tftp_handler handler[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_timeout_remove(sqe, (size_t) &handler->timeout_job, 0);
    io_uring_sqe_set_data(sqe, &handler->remove_timeout_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting remove timeout request: %s", strerror(-ret));
        exit(1);
    }
}

static void on_timeout(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    handler->is_timeout_job_pending = false;
    switch (-ret) {
        case ETIME:
            break;
        case ENOENT: // io_uring internal race conditions may occur resulting in ENOENT
            [[fallthrough]];
        case ECANCELED:
            if (handler->state == TFTP_HANDLER_STATE_AFTER_CLOSED) {
                resume_execution(handler);
            }
            return;
        default:
            logger_log_error(handler->logger, "Error while handling timeout: %s", strerror(-ret));
            return;
            exit(1);
    }
    if (handler->retransmits < handler->retries) {
        logger_log_debug(handler->logger, "ACK timeout for peer %s:%d. Retransmission no %d.", handler->stats.peer_addr, handler->stats.peer_port, handler->retransmits + 1);
        handler->retransmits += 1;
        handler->global_retransmits += 1;
        if (handler->valid_options_required && !handler->options_acknowledged) {
            submit_transmit_oack_request(handler);
        }
        else {
            handler->next_data_packet_to_send = handler->window_begin;
            submit_on_transmit_data_request(handler);
        }
        return;
    }
    static const char format[] = "timeout after %d retransmits.";
    static const char format_miss_last_ack[] = "timeout after %d retransmits. Missed last ack.";
    char *error_msg = malloc(sizeof(format_miss_last_ack) + 3 * sizeof(handler->retransmits) + 2);
    sprintf(error_msg, handler->waiting_last_ack ? format : format_miss_last_ack, handler->retransmits);
    handler->stats.error.error_occurred = true;
    handler->stats.error.error_number = TFTP_ERROR_NOT_DEFINED;
    handler->stats.error.error_message = error_msg;
    handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
    resume_execution(handler);
}

static void on_timeout_removed(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    handler->state = TFTP_HANDLER_STATE_AFTER_CLOSED;
    if (-ret == ENOENT) {
        resume_execution(handler);
        return;
    }
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while removing timeout: %s", strerror(-ret));
        exit(1);
    }
}

static void submit_on_next_block_request(struct tftp_handler handler[static 1]) {
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
    
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_readv(sqe,
                        handler->file_descriptor,
                        handler->file_iovec,
                        available_packets,
                        -1);
    handler->aio_job.routine = on_next_block;
    io_uring_sqe_set_data(sqe, &handler->aio_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting read request: %s", strerror(-ret));
        exit(1);
    }
}

static void on_next_block(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    if (ret < 0) {
        logger_log_warn(handler->logger, "Error while reading from source: %s", strerror(-ret));
        handler->stats.error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_NOT_DEFINED,
            .error_message = "Error while reading from source"
        };
        handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
        resume_execution(handler);
        return;
    }
    size_t bytes_read = ret;
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
        submit_on_next_block_request(handler);
        return;
    }
    handler->state = TFTP_HANDLER_STATE_SEND_DATA_PACKET;
    resume_execution(handler);
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
    handler->state = TFTP_HANDLER_STATE_SEND_DATA_PACKET;
    resume_execution(handler);
}

static void submit_on_transmit_data_request(struct tftp_handler handler[1]) {
    size_t next_data_packet_index = (handler->next_data_packet_to_send - 1) % handler->window_size;
    size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
    struct tftp_data_packet *packet = (struct tftp_data_packet *) ((uint8_t *) handler->data_packets + offset);
    bool is_last_packet = handler->next_data_packet_to_send == handler->next_data_packet_to_make - 1;
    size_t packet_size = sizeof *handler->data_packets + (is_last_packet ? handler->last_block_size : handler->block_size);
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_sendto(sqe,
                         handler->listener.file_descriptor,
                         packet,
                         packet_size,
                         0,
                         (struct sockaddr *) &handler->listener.peer_addr_storage,
                         handler->listener.peer_addrlen);
    handler->aio_job.routine = on_transmit_data;
    io_uring_sqe_set_data(sqe, &handler->aio_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting transmit data request: %s", strerror(-ret));
        exit(1);
    }
}

static void on_transmit_data(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while sending data: %s", strerror(-ret));
        exit(1);
    }
    logger_log_trace(handler->logger, "Sent DATA <block=%d, size=%zu bytes> to %s:%d", handler->next_data_packet_to_send, handler->block_size, handler->stats.peer_addr, handler->stats.peer_port);
    handler->stats.packets_sent += 1;
    handler->next_data_packet_to_send += 1;
    bool is_last_packet = (handler->next_data_packet_to_send == handler->next_data_packet_to_make) &&
                          (handler->last_block_size < handler->block_size);
    if (is_last_packet) {
        handler->waiting_last_ack = true;
        handler->state = TFTP_HANDLER_STATE_WAIT_ACK_PACKET;
    }
    else {
        bool is_packet_last_in_window = handler->next_data_packet_to_send - handler->window_size == handler->window_begin;
        handler->state = is_packet_last_in_window ? TFTP_HANDLER_STATE_WAIT_ACK_PACKET
                                                  : TFTP_HANDLER_STATE_SEND_DATA_PACKET;
    }
    resume_execution(handler);
}

static void submit_on_transmit_error_request(struct tftp_handler handler[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    size_t packet_len = sizeof(struct tftp_error_packet) + strlen(handler->stats.error.error_message) + 1;
    struct tftp_error_packet *packet = malloc(packet_len);
    memcpy(packet,
           &(struct tftp_error_packet) {
               .opcode = htons(TFTP_OPCODE_ERROR),
               .error_code = htons(handler->stats.error.error_number)
           },
           sizeof *packet);
    strcpy((char *) packet->error_message, handler->stats.error.error_message);
    io_uring_prep_sendto(sqe,
                         handler->listener.file_descriptor,
                         packet,
                         packet_len,
                         0,
                         (struct sockaddr *) &handler->listener.peer_addr_storage,
                         handler->listener.peer_addrlen);
    handler->aio_job.routine = on_transmit_error;
    io_uring_sqe_set_data(sqe, &handler->aio_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting transmit error request: %s", strerror(-ret));
        exit(1);
    }
}

static void on_transmit_error(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    // free(packet);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while sending error: %s", strerror(-ret));
        exit(1);
    }
    logger_log_trace(handler->logger, "Sent ERROR <message=%s> to %s:%d", handler->stats.error.error_message, handler->stats.peer_addr, handler->stats.peer_port);
    handler->state = TFTP_HANDLER_STATE_CLOSE;
    resume_execution(handler);
}

static void submit_transmit_oack_request(struct tftp_handler handler[static 1]) {
    size_t options_values_length = 0;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (handler->options[option].is_active) {
            options_values_length += strlen(tftp_option_recognized_string[option]) + 1;
            options_values_length += strlen(handler->options[option].value) + 1;
        }
    }
    size_t packet_len = sizeof(struct tftp_oack_packet) + options_values_length;
    struct tftp_oack_packet *packet = malloc(packet_len);
    packet->opcode = htons(TFTP_OPCODE_OACK);
    unsigned char *ptr = packet->options_values;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (handler->options[option].is_active) {
            size_t len = strlen(tftp_option_recognized_string[option]) + 1;
            ptr = (unsigned char *) memcpy(ptr, tftp_option_recognized_string[option], len) + len;
            len = strlen(handler->options[option].value) + 1;
            ptr = (unsigned char *) memcpy(ptr, handler->options[option].value, len) + len;
        }
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(handler->ring);
    if (sqe == nullptr) {
        logger_log_error(handler->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_sendto(sqe,
                         handler->listener.file_descriptor,
                         packet,
                         packet_len,
                         0,
                         (struct sockaddr *) &handler->listener.peer_addr_storage,
                         handler->listener.peer_addrlen);
    handler->aio_job.routine = on_transmit_oack;
    io_uring_sqe_set_data(sqe, &handler->aio_job);
    int ret = io_uring_submit(handler->ring);
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while submitting transmit OACK request: %s", strerror(-ret));
        exit(1);
    }
}

static void on_transmit_oack(int32_t ret, void *args) {
    struct tftp_handler *handler = args;
    if (ret < 0) {
        logger_log_error(handler->logger, "Error while sending OACK: %s", strerror(-ret));
        exit(1);
    }
    logger_log_trace(handler->logger, "Sent OACK %s to %s:%d", handler->stats.options_acked, handler->stats.peer_addr, handler->stats.peer_port);
    // free(packet);
    handler->stats.packets_sent += 1;
    handler->state = TFTP_HANDLER_STATE_WAIT_ACK_PACKET;
    resume_execution(handler);
}

static void close_handler(struct tftp_handler handler[static 1]) {
    handler->stats.retransmits = handler->global_retransmits;
    handler->stats_callback(&handler->stats);
    logger_log_debug(handler->logger, "Closing response data object.");
    close(handler->file_descriptor);
    logger_log_debug(handler->logger, "Closing socket.");
    close(handler->listener.file_descriptor);
    logger_log_debug(handler->logger, "Session closed.");
    if (handler->is_timeout_job_pending) {
        submit_remove_timeout_request(handler);
    }
}

static void parse_options(struct tftp_handler handler[static 1]) {
    if (handler->options_str == nullptr) {
        logger_log_info(handler->logger, "No options requested from peer %s:%d.", handler->stats.peer_addr, handler->stats.peer_port);
        return;
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
                    handler->timeout_timespec.tv_sec = handler->timeout;
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
                        handler->state = TFTP_HANDLER_STATE_SEND_ERROR_PACKET;
                        resume_execution(handler);
                        return;
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
        char *full_path = malloc(strlen(root) + strlen(handler->path) + 1);
        strcpy(full_path, root);
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
