#include "session.h"

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <logger.h>

#include "dispatcher.h"
#include "session_file.h"
#include "../utils/inet.h"

enum event : uint32_t {
    EVENT_START,
    EVENT_DATA_AVAILABLE,
    EVENT_PACKET_RECEIVED,
    EVENT_PACKET_RECEIVED_REMOVED,
    EVENT_TIMEOUT,
    EVENT_TIMEOUT_REMOVED,
};

static inline struct __kernel_timespec timespec_to_kernel_timespec(struct timespec ts) {
    return (struct __kernel_timespec) {.tv_sec = ts.tv_sec, .tv_nsec = ts.tv_nsec};
}

struct tftp_data_packet_info {
    struct tftp_data_packet *packet;
    uint16_t packet_size;
};

static struct tftp_data_packet_info get_data_packet_info(struct tftp_session session[static 1], uint16_t i);

static bool fetch_data_octet_async(struct tftp_session session[static 1]);
static bool recv_async(struct tftp_session session[static 1]);
static bool recv_async_cancel(struct tftp_session session[static 1]);
static bool submit_timeout(struct tftp_session session[static 1]);
static bool submit_cancel_timeout(struct tftp_session session[static 1]);

static bool start(struct tftp_session session[static 1]);
static void close_session(struct tftp_session session[static 1]);
static bool on_data_available(struct tftp_session session[static 1], struct dispatcher_event event[static 1]);
static bool on_timeout(struct tftp_session session[static 1]);
static bool on_packet_received(struct tftp_session session[static 1], struct dispatcher_event event[static 1]);

static bool send_oack(struct tftp_session session[static 1]);
static bool set_address_family(struct inet_address address[static 1]);
static bool is_request_valid(struct tftp_session session[static 1]);
static bool oack_packet_init(struct tftp_session session[static 1]);
static bool error_packet_init(struct tftp_session session[static 1]);
static bool fetch_data_netascii_async(struct tftp_session session[static 1]);
static bool create_data_packets(struct tftp_session session[static 1], size_t bytes_read);
static bool report_client_error(struct tftp_session session[static 1], size_t error_packet_size);
static bool set_max_retransmissions_error(struct tftp_session session[static 1]);

static bool end_of_file(struct tftp_session session[static 1], size_t bytes_read);

static inline bool is_in_range(uint16_t n, uint16_t begin, uint16_t end) {
    return begin <= end ? (begin <= n && n <= end) : (begin <= n || n <= end);
}

static inline size_t get_cumulative_ackd_payload_size(struct tftp_session session[static 1], uint16_t block_number) {
    return session->block_size * ((uint16_t) (block_number - session->window_begin)) +
           (block_number == session->last_packet ? session->last_block_size : session->block_size);
}

static inline enum event get_event(struct dispatcher_event event[static 1]) {
    return (enum event) (event->id & 0xFFFF);
}

void tftp_session_init(struct tftp_session session[static 1],
                       uint16_t session_id,
                       struct tftp_server_info server_info[static 1],
                       struct dispatcher dispatcher[static 1],
                       struct logger logger[static 1]) {
    *session = (struct tftp_session) {
        .server_info = server_info,
        .dispatcher = dispatcher,
        .logger = logger,
        .connection = { .sockfd = -1, },
        .event_start = {.id = ((uint64_t) session_id << 48) | EVENT_START},
        .event_timeout = {.event = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT}},
        .event_cancel_timeout = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT_REMOVED},
        .event_packet_received = {.id = ((uint64_t) session_id << 48) | EVENT_PACKET_RECEIVED},
        .event_cancel_packet_received = {.id = ((uint64_t) session_id << 48) | EVENT_PACKET_RECEIVED_REMOVED},
        .event_next_block = {.id = ((uint64_t) session_id << 48) | EVENT_DATA_AVAILABLE},
        .oack_packet = nullptr,
        .error_packet = nullptr,
        .data_packets = nullptr,
        .retries = server_info->retries,
        .timeout = server_info->timeout,
        .block_size = tftp_default_blksize,
        .window_size = 1,
        .netascii_buffer = -1,
        .last_packet = -1,
        .window_begin = 1,
        .next_data_packet_to_send = 1,
        .expected_sequence_number = 1,
    };
}

enum tftp_session_state tftp_session_handle_event(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    enum event e = get_event(event);
    switch (e) {
        case EVENT_START:
            if (!start(session)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            if (session->should_close) {
                break;
            }
            if (session->options.valid_options_required && !session->options.options_acknowledged) {
                if (!send_oack(session)) {
                    return TFTP_SESSION_STATE_ERROR;
                }
            }
            else if (session->request_type == SESSION_WRITE_REQUEST) {
                const struct tftp_ack_packet ack_packet = {
                    .opcode = htons(TFTP_OPCODE_ACK),
                    .block_number = 0,
                };
                if (!sendto(session->connection.sockfd, &ack_packet, sizeof ack_packet, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen)) {
                    return TFTP_SESSION_STATE_ERROR;
                }
                logger_log_trace(session->logger, "Sent ACK <block=0> to %s:%d", session->connection.client_address.str, session->connection.client_address.port);
            }
            if (!recv_async(session)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            break;
        case EVENT_TIMEOUT:
            session->pending_jobs--;
            if (!event->is_success && (event->error_number == ENOENT || event->error_number == ECANCELED)) {
                break;
            }
            if (session->should_close || !session->is_timer_active) {
                break;
            }
            if (!on_timeout(session)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            break;
        case EVENT_TIMEOUT_REMOVED:
            session->pending_jobs--;
            if (!event->is_success && event->error_number != 0 && event->error_number != ENOENT && event->error_number != EALREADY) {
                logger_log_error(session->logger, "Error while removing timeout: %s", strerror(event->error_number));
                return TFTP_SESSION_STATE_ERROR;
            }
            break;
        case EVENT_PACKET_RECEIVED:
            session->pending_jobs--;
            if (session->should_close || (!event->is_success && event->error_number == ECANCELED)) {
                break;
            }
            if (!on_packet_received(session, event)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            if (session->should_close) {
                break;
            }
            if (!recv_async(session)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            if (!session->options.options_acknowledged && session->options.valid_options_required) {
                break;
            }
            break;
        case EVENT_PACKET_RECEIVED_REMOVED:
            session->pending_jobs--;
            if (!event->is_success && event->error_number != 0 && event->error_number != ENOENT) {
                logger_log_error(session->logger, "Error while removing packet received event: %s", strerror(event->error_number));
                return TFTP_SESSION_STATE_ERROR;
            }
            break;
        case EVENT_DATA_AVAILABLE:
            session->pending_jobs--;
            session->is_fetching_data = false;
            if (!on_data_available(session, event)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            if (session->should_close) {
                break;
            }
            break;
        default:
            logger_log_error(session->logger, "Unknown event id: %lu", e);
            return TFTP_SESSION_STATE_ERROR;
    }
    bool should_fetch_data = session->request_type == SESSION_READ_REQUEST
                             && !session->is_fetching_data
                             && !session->should_close
                             && (!session->options.valid_options_required || session->options.options_acknowledged)
                             && session->last_packet == -1
                             && is_in_range(session->next_data_packet_to_send,
                                            session->window_begin,
                                            session->window_begin + session->window_size - 1);
    if (should_fetch_data) {
        session->is_fetching_data = true;
        auto fetch_data_async = session->mode == TFTP_MODE_OCTET ? fetch_data_octet_async : fetch_data_netascii_async;
        if (!fetch_data_async(session)) {
            return TFTP_SESSION_STATE_ERROR;
        }
    }
    if (session->should_close) {
        logger_log_trace(session->logger, "Waiting for %d pending jobs to finish.", session->pending_jobs);
    }
    if (session->should_close && session->pending_jobs == 0) {
        logger_log_debug(session->logger, "Closing session.");
        session->stats.retransmits = session->total_retransmissions;
        if (session->stats.callback != nullptr) {
            session->stats.callback(&session->stats);
        }
        close_session(session);
        return TFTP_SESSION_STATE_CLOSED;
    }
    return TFTP_SESSION_STATE_IDLE;
}

static bool send_oack(struct tftp_session session[static 1]) {
    if (!oack_packet_init(session)) {
        logger_log_error(session->logger, "Could not initialize OACK packet.");
        return false;
    }
    if (session->is_adaptive_timeout_active) {
        adaptive_timeout_start_timer(&session->adaptive_timeout);
        session->adaptive_timeout.starting_block_number = 0;
    }
    if (!submit_timeout(session)) {
        return false;
    }
    ssize_t ret = sendto(session->connection.sockfd, session->oack_packet, session->oack_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
    if (ret == -1) {
        logger_log_error(session->logger, "Error while sending OACK: %s", strerror(errno));
        return false;
    }
    logger_log_trace(session->logger, "Sent OACK %s to %s:%d", session->stats.options_acked, session->connection.client_address.str, session->connection.client_address.port);
    return true;
}

static bool start(struct tftp_session session[static 1]) {
    logger_log_trace(session->logger, "Starting session.");
    logger_log_trace(session->logger, "Validating request.");
    if (!is_request_valid(session)) {
        logger_log_warn(session->logger, "Invalid request.");
        session->should_close = true;
        return true;
    }
    logger_log_trace(session->logger, "Request validated.");
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(session->request_args.buffer);
    if (opcode == TFTP_OPCODE_WRQ) {
        session->request_type = SESSION_WRITE_REQUEST;
    }
    if (opcode == TFTP_OPCODE_RRQ) {
        session->request_type = SESSION_READ_REQUEST;
    }
    
    if (!session_connection_init(&session->connection,
                                 session->server_info->server_addrinfo,
                                 session->request_args.peer_addr,
                                 session->request_args.peer_addrlen,
                                 session->request_args.is_orig_dest_addr_ipv4,
                                 session->logger)) {
        logger_log_error(session->logger, "Could not initialize session connection socket. Ignoring request.");
        return false;
    }
    
    const char *filename = (char *)&session->request_args.buffer[2];
    if (session->connection.client_address.str != nullptr) {
        const char *request_str = session->request_type == SESSION_READ_REQUEST ? "to read" : "to write";
        logger_log_info(session->logger, "New incoming connection from client '%s:%d' asking %s file '%s'", session->connection.client_address.str, session->connection.client_address.port, request_str, filename);
    }
    
    session->stats = tftp_session_stat_init(session->connection.address.sockaddr,
                                            session->connection.client_address.sockaddr,
                                            filename,
                                            session->server_info->session_stats_callback,
                                            session->logger);
    
    bool ret = session_options_init(&session->options,
                                    session->request_args.bytes_recvd - 2,
                                    (const char *) &session->request_args.buffer[2],
                                    &session->filename,
                                    &session->mode,
                                    &session->timeout,
                                    &session->block_size,
                                    &session->window_size,
                                    &session->is_adaptive_timeout_active,
                                    &session->stats.error);
    session->stats.mode = tftp_mode_to_string(session->mode);
    if (!ret) {
        return true;    // send error and close session
    }
    enum tftp_read_type read_type = TFTP_READ_TYPE_FILE;
    if (session->options.options_str != nullptr
        && session->server_info->is_list_request_enabled
        && session->request_type == SESSION_READ_REQUEST) {
        read_type = session_options_get_read_type(&session->options);
    }

    enum session_file_mode file_mode = session->request_type == SESSION_READ_REQUEST ? SESSION_FILE_MODE_READ : SESSION_FILE_MODE_WRITE;
    session->file_descriptor = session_file_init(session->filename, session->server_info->root, file_mode, read_type, &session->stats.error);
    if (session->file_descriptor == -1) {
        return true;    // send error and close session
    }
    if (session->options.options_str == nullptr) {
        logger_log_info(session->logger, "No options requested from peer %s:%d.", session->stats.peer_addr, session->stats.peer_port);
    }
    else {
        tftp_format_option_strings(session->options.options_str_size, session->options.options_str, session->stats.options_in);
        logger_log_info(session->logger, "Options requested from peer %s:%d are [%s]", session->stats.peer_addr, session->stats.peer_port, session->stats.options_in);
        if (!parse_options(&session->options, session->file_descriptor, session->server_info->is_adaptive_timeout_enabled, session->server_info->is_list_request_enabled)) {
            return true;
        }
        tftp_format_options(session->options.recognized_options, session->stats.options_acked);
        logger_log_info(session->logger, "Options to ack for peer %s:%d are %s", session->stats.peer_addr, session->stats.peer_port, session->stats.options_acked);
        session->stats.blksize = session->block_size;
        if (session->request_type == SESSION_READ_REQUEST) {
            session->stats.window_size = session->window_size;
        }
    }
    if (session->is_adaptive_timeout_active) {
        adaptive_timeout_init(&session->adaptive_timeout);
    }
    session->data_packets = malloc(session->window_size * (sizeof *session->data_packets + session->block_size));
    if (session->data_packets == nullptr) {
        logger_log_error(session->logger, "Could not initialize DATA packets storage. Not enough memory: %s.", strerror(errno));
        return false;
    }
    size_t recv_buffer_size = sizeof(struct tftp_data_packet) + (session->block_size < tftp_default_blksize ? tftp_default_blksize : session->block_size);
    void *recv_buffer = realloc(session->connection.recv_buffer, recv_buffer_size);
    if (recv_buffer == nullptr) {
        logger_log_error(session->logger, "Could not initialize receive buffer. Not enough memory: %s.", strerror(errno));
        return false;
    }
    session->connection.recv_buffer = recv_buffer;
    session->connection.recv_buffer_size = recv_buffer_size;
    
    session->event_timeout.timeout.tv_sec = session->timeout;
    if (session->stats.error.error_occurred) {
        if (!error_packet_init(session)) {
            logger_log_error(session->logger, "Could not initialize error packet.");
            return false;
        }
        if (sendto(session->connection.sockfd, session->error_packet, session->error_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen) == -1) {
            logger_log_error(session->logger, "Error while sending ERROR: %s", strerror(errno));
            return false;
        }
        logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, session->connection.last_message_address.str, session->connection.last_message_address.port);
        session->should_close = true;
        return true;
    }
    return true;
}

static bool on_data_available(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    if (session->mode == TFTP_MODE_OCTET) {
        while (true) {
            if (!event->is_success) {
                session->should_close = true;
                logger_log_warn(session->logger, "Error while reading from source: %s", strerror(event->error_number));
                session->stats.error = (struct tftp_session_stats_error) {
                    .error_occurred = true,
                    .error_number = TFTP_ERROR_NOT_DEFINED,
                    .error_message = "Error while reading from source"
                };
                if (!error_packet_init(session)) {
                    logger_log_error(session->logger, "Could not initialize error packet.");
                    return false;
                }
                ssize_t ret = sendto(session->connection.sockfd, session->error_packet, session->error_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
                if (ret == -1) {
                    logger_log_error(session->logger, "Error while sending ERROR: %s", strerror(errno));
                    return false;
                }
                logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, session->connection.last_message_address.str, session->connection.last_message_address.port);
                return true;
            }
            if (!create_data_packets(session, event->result)) {
                return true;
            }
            logger_log_trace(session->logger, "Created DATA packets.");
            break;
        }
    }
    if (session->is_adaptive_timeout_active) {
        if (!session->adaptive_timeout.is_timer_active) {
            adaptive_timeout_start_timer(&session->adaptive_timeout);
            session->adaptive_timeout.starting_block_number = session->next_data_packet_to_send;
        }
    }
    auto packet_info = get_data_packet_info(session, session->next_data_packet_to_send);
    const size_t packet_size = sizeof(struct tftp_data_packet) + session->last_block_size;
    ssize_t ret = sendto(session->connection.sockfd, packet_info.packet, packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
    if (ret == -1) {
        logger_log_error(session->logger, "Error while sending data: %s", strerror(errno));
        return false;
    }
    session->stats.packets_sent += 1;
    uint16_t block_number = ntohs(packet_info.packet->block_number);
    logger_log_trace(session->logger, "Sent DATA <block=%d, size=%zu bytes> to %s:%d", block_number, ret - sizeof(struct tftp_data_packet), session->connection.client_address.str, session->connection.client_address.port);
    if (session->last_block_size < session->block_size) {
        session->last_packet = session->next_data_packet_to_send;
    }
    if (session->window_begin == session->next_data_packet_to_send) {
        if (!submit_timeout(session)) {
            return false;
        }
    }
    session->next_data_packet_to_send += 1;
    return true;
}

static bool on_timeout(struct tftp_session session[static 1]) {
    if (session->current_retransmission >= session->retries) {
        session->should_close = true;
        if (!recv_async_cancel(session)) {
            return false;
        }
        if (!set_max_retransmissions_error(session)) {
            return false;
        }
        if (!error_packet_init(session)) {
            logger_log_error(session->logger, "Could not initialize error packet.");
            return false;
        }
        ssize_t ret = sendto(session->connection.sockfd, session->error_packet, session->error_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
        if (ret == -1) {
            logger_log_error(session->logger, "Error while sending data: %s", strerror(errno));
            return false;
        }
        logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, session->connection.client_address.str, session->connection.client_address.port);
        return true;
    }
    logger_log_debug(session->logger, "Timeout for client %s:%d. Retransmission no %d.", session->connection.client_address.str, session->connection.client_address.port, session->current_retransmission + 1);
    session->current_retransmission += 1;
    session->total_retransmissions += 1;
    if (session->is_adaptive_timeout_active) {
        adaptive_timeout_cancel_timer(&session->adaptive_timeout);
        adaptive_timeout_backoff(&session->adaptive_timeout);
        auto timeout = timespec_to_kernel_timespec(session->adaptive_timeout.rto);
        session->event_timeout.timeout = timeout;
        logger_log_trace(session->logger, "Adaptive timeout: RTO set to %lld.%.9ld seconds via exponential backoff.", timeout.tv_sec, timeout.tv_nsec);
    }
    if (!submit_timeout(session)) {
        return false;
    }
    if (session->options.valid_options_required && !session->options.options_acknowledged) {
        ssize_t ret = sendto(session->connection.sockfd, session->oack_packet, session->oack_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
        if (ret == -1) {
            logger_log_error(session->logger, "Error while sending OACK: %s", strerror(errno));
            return false;
        }
        logger_log_trace(session->logger, "Sent OACK %s to %s:%d", session->stats.options_acked, session->connection.client_address.str, session->connection.client_address.port);
        return true;
    }
    if (session->request_type == SESSION_WRITE_REQUEST) {
        const struct tftp_ack_packet ack_packet = {
            .opcode = htons(TFTP_OPCODE_ACK),
            .block_number = htons(session->expected_sequence_number - 1),
        };
        ssize_t ret = sendto(session->connection.sockfd, &ack_packet, sizeof ack_packet, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
        if (ret == -1) {
            logger_log_error(session->logger, "Error while sending ACK: %s", strerror(errno));
            return false;
        }
        logger_log_trace(session->logger, "Sent ACK <block=%d> to %s:%d", ntohs(ack_packet.block_number), session->connection.client_address.str, session->connection.client_address.port);
        return true;
    }
    logger_log_trace(session->logger, "Retransmitting DATA packets in window [%d, %d].", session->window_begin, (uint16_t) session->next_data_packet_to_send - 1);
    for (uint16_t i = session->window_begin; is_in_range(i, session->window_begin, session->next_data_packet_to_send - 1); i++) {
        auto packet_info = get_data_packet_info(session, i);
        ssize_t ret = sendto(session->connection.sockfd, packet_info.packet, packet_info.packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
        if (ret == -1) {
            logger_log_error(session->logger, "Error while sending data: %s", strerror(errno));
            return false;
        }
        if (ret < packet_info.packet_size) {
            // TODO: handle tsize option
            logger_log_warn(session->logger, "Could not send whole packet. The file transfer failed but the client will not be able to detect it.");
            return true;
        }
        logger_log_trace(session->logger, "Sent DATA <block=%d, size=%zu bytes> to %s:%d", ntohs(packet_info.packet->block_number), packet_info.packet_size - sizeof *session->data_packets, session->connection.client_address.str, session->connection.client_address.port);
    }
    return true;
}

static bool on_packet_received(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    auto sender_address = &session->connection.last_message_address;
    if (!event->is_success) {
        logger_log_error(session->logger, "Error while receiving data: %s", strerror(event->error_number));
        return false;
    }
    if (event->result < 4) {
        logger_log_warn(session->logger, "Received packet is too short. Ignoring packet.");
        return true;
    }
    if (!set_address_family(sender_address)) {
        logger_log_warn(session->logger, "Unknown address family: %d. Ignoring packet.", sender_address->storage.ss_family);
        return true;
    }
    if (!sockaddr_equals(&sender_address->storage, &session->connection.client_address.storage)) {
        if (sender_address->str == nullptr) {
            logger_log_error(session->logger, "Unexpected sender, could not translate client address to a string representation.");
        }
        else {
            logger_log_warn(session->logger, "Unexpected sender: '%s:%d', expected client: '%s:%d'.", sender_address->str, sender_address->port, session->connection.client_address.str, session->connection.client_address.port);
        }
        session->stats.error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_UNKNOWN_TRANSFER_ID,
            .error_message = "Unknown transfer ID.",
        };
        if (!error_packet_init(session)) {
            logger_log_error(session->logger, "Could not initialize error packet.");
            return false;
        }
        ssize_t ret = sendto(session->connection.sockfd, session->error_packet, session->error_packet_size, 0, sender_address->sockaddr, sender_address->addrlen);
        if (ret == -1) {
            logger_log_error(session->logger, "Error while sending data: %s", strerror(errno));
            return false;
        }
        logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, sender_address->str, sender_address->port);
        session->stats.error.error_occurred = false;
        return true;
    }
    enum tftp_opcode opcode = ntohs(*(uint16_t *) session->connection.recv_buffer);
    switch (opcode) {
        case TFTP_OPCODE_ERROR: {
            session->should_close = true;
            return report_client_error(session, event->result);
        }
        case TFTP_OPCODE_DATA: {
            if (session->request_type == SESSION_READ_REQUEST) {
                break;
            }
            struct tftp_data_packet *data_packet = (struct tftp_data_packet *) session->connection.recv_buffer;
            uint16_t block_number = ntohs(data_packet->block_number);
            if (!session->options.options_acknowledged && session->options.valid_options_required && block_number == 1) {
                logger_log_trace(session->logger, "Options acknowledged.");
                session->options.options_acknowledged = true;
            }
            else if (block_number != session->expected_sequence_number) {
                logger_log_trace(session->logger, "Received unexpected DATA <block=%d> from %s:%d, expected %d. Ignoring packet.", block_number, session->connection.client_address.str, session->connection.client_address.port, session->expected_sequence_number);
                return true;
            }
            size_t data_size = event->result - sizeof(struct tftp_data_packet);
            logger_log_trace(session->logger, "Received DATA <block=%d, size=%zu bytes> from %s:%d", block_number, data_size, session->connection.client_address.str, session->connection.client_address.port);
            ssize_t bytes_written = write(session->file_descriptor, data_packet->data, data_size);
            if (bytes_written == -1) {
                logger_log_error(session->logger, "Error while writing to file: %s", strerror(errno));
                session->stats.error = (struct tftp_session_stats_error) {
                    .error_occurred = true,
                    .error_number = TFTP_ERROR_NOT_DEFINED,
                    .error_message = "Error writing to disk",
                };
                return false;
            }
            session->stats.bytes_sent += bytes_written;
            const struct tftp_ack_packet ack_packet = {
                .opcode = htons(TFTP_OPCODE_ACK),
                .block_number = htons(block_number),
            };
            if (sendto(session->connection.sockfd, &ack_packet, sizeof ack_packet, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen) == -1) {
                logger_log_error(session->logger, "Error while sending ACK: %s", strerror(errno));
                return false;
            }
            logger_log_trace(session->logger, "Sent ACK <block=%d> to %s:%d", block_number, session->connection.client_address.str, session->connection.client_address.port);
            session->expected_sequence_number++;
            session->stats.packets_acked++;
            session->current_retransmission = 0;
            session->should_close = (data_size < session->block_size);
            return true;
        }
        case TFTP_OPCODE_ACK: {
            if (session->request_type == SESSION_WRITE_REQUEST) {
                break;
            }
            uint16_t block_number = ntohs(*(uint16_t *) &session->connection.recv_buffer[2]);
            if (!session->options.options_acknowledged && session->options.valid_options_required && block_number == 0) {
                session->options.options_acknowledged = true;
            }
            else if (!is_in_range(block_number, session->window_begin, session->next_data_packet_to_send - 1)) {
                logger_log_trace(session->logger, "Received unexpected ACK <block=%d> from %s:%d not in window [%d, %d]. Ignoring packet.", block_number, session->connection.client_address.str, session->connection.client_address.port, session->window_begin, session->next_data_packet_to_send - 1);
                return true;
            }
            else {
                session->stats.bytes_sent += get_cumulative_ackd_payload_size(session, block_number);
            }
            logger_log_trace(session->logger, "Received ACK <block=%d> from %s:%d", block_number, session->connection.client_address.str, session->connection.client_address.port);
            session->window_begin = block_number + 1;
            
            if (session->is_adaptive_timeout_active
                && session->adaptive_timeout.is_timer_active
                && is_in_range(block_number, session->adaptive_timeout.starting_block_number, session->next_data_packet_to_send - 1)) {
                adaptive_timeout_stop_timer(&session->adaptive_timeout);
                auto timeout = timespec_to_kernel_timespec(session->adaptive_timeout.rto);
                session->event_timeout.timeout = timeout;
                logger_log_trace(session->logger, "Adaptive timeout: RTO set to %lld.%.9ld seconds.", timeout.tv_sec, timeout.tv_nsec);
            }
            if (!submit_cancel_timeout(session)) {
                return false;
            }
            // if packets to retransmit still exists restart timeout
            if (session->window_begin != session->next_data_packet_to_send) {
                if (!submit_timeout(session)) {
                    return false;
                }
            }
            
            session->stats.packets_acked += 1;
            session->current_retransmission = 0;
            session->should_close = (session->last_packet != -1) && (block_number == session->last_packet);
            return true;
        }
    }
    
    logger_log_error(session->logger, "Expected %s opcode from %s:%d, got: %hu.",  session->request_type == SESSION_READ_REQUEST ? "ACK" : "DATA", session->connection.client_address.str, session->connection.client_address.port, opcode);
    session->should_close = true;
    session->stats.error = (struct tftp_session_stats_error) {
        .error_occurred = true,
        .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
        .error_message = "Unexpected packet opcode.",
    };
    if (!error_packet_init(session)) {
        logger_log_error(session->logger, "Could not initialize error packet.");
        return false;
    }
    ssize_t ret = sendto(session->connection.sockfd, session->error_packet, session->error_packet_size, 0, session->connection.client_address.sockaddr, session->connection.client_address.addrlen);
    if (ret == -1) {
        logger_log_error(session->logger, "Error while sending data: %s", strerror(errno));
        return false;
    }
    logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, session->connection.client_address.str, session->connection.client_address.port);
    return true;
}

static void close_session(struct tftp_session session[static 1]) {
    if (session->file_descriptor != -1) {
        close(session->file_descriptor);
    }
    if (session->connection.sockfd != -1) {
        session_connection_destroy(&session->connection, session->logger);
    }
    free(session->oack_packet);
    free(session->error_packet);
    free(session->data_packets);
    logger_log_debug(session->logger, "Session closed.");
}

static bool set_address_family(struct inet_address address[static 1]) {
    switch (address->storage.ss_family) {
        case AF_INET:
            address->addrlen = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            address->addrlen = sizeof(struct sockaddr_in6);
            break;
        default:
            return false;
    }
    address->str = nullptr;
    if (sockaddr_ntop(address->sockaddr, address->str_storage, &address->port) != nullptr) {
        address->str = address->str_storage;
    }
    return true;
}

static struct tftp_data_packet_info get_data_packet_info(struct tftp_session session[static 1], uint16_t i) {
    const bool is_last_packet = (i == session->last_packet);
    const size_t offset = (((uint16_t) (i - 1)) % session->window_size) * (sizeof(struct tftp_data_packet) + session->block_size);
    auto const packet = (struct tftp_data_packet *) ((uint8_t *) session->data_packets + offset);
    const uint16_t packet_size = sizeof *session->data_packets + (is_last_packet ? session->last_block_size : session->block_size);
    return (struct tftp_data_packet_info) {
        .packet = packet,
        .packet_size = packet_size
    };
}

static bool is_request_valid(struct tftp_session session[static 1]) {
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(session->request_args.buffer);
    if (opcode != TFTP_OPCODE_RRQ && opcode != TFTP_OPCODE_WRQ) {
        logger_log_warn(session->logger, "Unexpected TFTP opcode %d.", opcode);
        return false;
    }
    if (opcode == TFTP_OPCODE_WRQ && !session->server_info->is_write_request_enabled) {
        logger_log_warn(session->logger, "Write request disabled.");
        return false;
    }
    size_t token_length = 0;
    size_t option_size = session->request_args.bytes_recvd - 2;
    const char *option_ptr = (const char *) &session->request_args.buffer[2];
    const char *end_ptr = &option_ptr[option_size - 1];
    while (option_ptr != nullptr && option_ptr < end_ptr) {
        option_ptr = memchr(option_ptr, '\0', end_ptr - option_ptr + 1);
        if (option_ptr != nullptr) {
            token_length++;
            option_ptr++;
        }
    }
    if (token_length < 2 || token_length % 2 != 0) {
        logger_log_warn(session->logger, "Received ill formed packet, ignoring (tokens length: %zu).", token_length);
        return false;
    }
    return true;
}

static bool fetch_data_octet_async(struct tftp_session session[static 1]) {
    const uint16_t packet_index = ((uint16_t) (session->next_data_packet_to_send - 1)) % session->window_size;
    const size_t offset = packet_index * (sizeof(struct tftp_data_packet) + session->block_size);
    struct tftp_data_packet *packet = (void *) ((uint8_t *) session->data_packets + offset);
    
    session->last_block_size = session->incomplete_read ? session->last_block_size : 0;
    void *data = &(packet->data)[session->last_block_size];
    const size_t block_size = session->block_size - session->last_block_size;
    session->incomplete_read = false;
    
    if (!dispatcher_submit_read(session->dispatcher,
                                &session->event_next_block,
                                session->file_descriptor,
                                data,
                                block_size)) {
        logger_log_error(session->logger, "Could not submit read request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

/**
 * This is comically bad but I don't plan to improve this in the near future.
 */
static bool fetch_data_netascii_async(struct tftp_session session[static 1]) {
    uint16_t next_data_packet_index = ((uint16_t) (session->next_data_packet_to_send - 1)) % session->window_size;
    size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + session->block_size);
    struct tftp_data_packet *packet = (void *) ((uint8_t *) session->data_packets + offset);
    uint8_t *block = packet->data;
    while (block - packet->data < session->block_size) {
        if (session->netascii_buffer != -1) {
            *block++ = session->netascii_buffer;
            session->netascii_buffer = -1;
        }
        ssize_t byte_read = read(session->file_descriptor, block, 1);
        if (byte_read == -1) {
            logger_log_error(session->logger, "Error while reading from source: %s", strerror(errno));
            session->stats.error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_NOT_DEFINED,
                .error_message = "Error while reading from source"
            };
            return false;
        }
        if (byte_read == 0) {
            break;
        }
        int c = *block++;
        if (c == '\n' || c == '\r') {
            if (block - packet->data == session->block_size) {
                session->netascii_buffer = c;
            }
            else {
                *(block++) = (c == '\n') ? '\r' : '\0';
            }
        }
    }
    tftp_data_packet_init(packet, session->next_data_packet_to_send);
    session->last_block_size = block - packet->data;
    if (!dispatcher_submit(session->dispatcher, &session->event_next_block)) {
        logger_log_error(session->logger, "Could not submit nop request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool recv_async(struct tftp_session session[static 1]) {
    session->connection.last_message_address.addrlen = sizeof session->connection.last_message_address.storage;
    session->connection.iovec[0].iov_base = session->connection.recv_buffer;
    session->connection.iovec[0].iov_len = session->connection.recv_buffer_size;
    session->connection.msghdr.msg_name = &session->connection.last_message_address.storage;
    session->connection.msghdr.msg_namelen = session->connection.last_message_address.addrlen;
    bool ret = dispatcher_submit_recvmsg(session->dispatcher,
                                         &session->event_packet_received,
                                         session->connection.sockfd,
                                         &session->connection.msghdr,
                                         0);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting receive new data request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool recv_async_cancel(struct tftp_session session[static 1]) {
    bool ret = dispatcher_submit_cancel(session->dispatcher, &session->event_cancel_packet_received, &session->event_packet_received);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting cancel receive new data request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool submit_timeout(struct tftp_session session[static 1]) {
    if (!dispatcher_submit_timeout(session->dispatcher, &session->event_timeout)) {
        logger_log_error(session->logger, "Could not submit timeout.");
        return false;
    }
    session->pending_jobs++;
    session->is_timer_active = true;
    return true;
}

static bool submit_cancel_timeout(struct tftp_session session[static 1]) {
    if (!dispatcher_submit_timeout_cancel(session->dispatcher, &session->event_cancel_timeout, &session->event_timeout)) {
        logger_log_error(session->logger, "Error while submitting cancel timeout request.");
        return false;
    }
    session->pending_jobs++;
    session->is_timer_active = false;
    return true;
}

static bool oack_packet_init(struct tftp_session session[static 1]) {
    size_t options_values_length = 0;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (session->options.recognized_options[option].is_active) {
            options_values_length += strlen(tftp_option_recognized_string[option]) + 1;
            options_values_length += strlen(session->options.recognized_options[option].value) + 1;
        }
    }
    ssize_t packet_len = sizeof(struct tftp_oack_packet) + options_values_length;
    session->oack_packet = malloc(packet_len);
    if (session->oack_packet == nullptr) {
        logger_log_error(session->logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    session->oack_packet->opcode = htons(TFTP_OPCODE_OACK);
    unsigned char *ptr = session->oack_packet->options_values;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (session->options.recognized_options[option].is_active) {
            size_t len = strlen(tftp_option_recognized_string[option]) + 1;
            ptr = (unsigned char *) memcpy(ptr, tftp_option_recognized_string[option], len) + len;
            len = strlen(session->options.recognized_options[option].value) + 1;
            ptr = (unsigned char *) memcpy(ptr, session->options.recognized_options[option].value, len) + len;
        }
    }
    session->oack_packet_size = packet_len;
    return true;
}

static bool error_packet_init(struct tftp_session session[static 1]) {
    free(session->error_packet);
    session->error_packet = nullptr;
    ssize_t packet_len = sizeof(struct tftp_error_packet) + strlen(session->stats.error.error_message) + 1;
    session->error_packet = malloc(packet_len);
    if (session->error_packet == nullptr) {
        logger_log_error(session->logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    memcpy(session->error_packet,
           &(struct tftp_error_packet) {
               .opcode = htons(TFTP_OPCODE_ERROR),
               .error_code = htons(session->stats.error.error_number)
           },
           sizeof *session->error_packet);
    strcpy((char *) session->error_packet->error_message, session->stats.error.error_message);
    session->error_packet_size = packet_len;
    return true;
}

static bool create_data_packets(struct tftp_session session[static 1], size_t bytes_read) {
    session->last_block_size += bytes_read;
    if (bytes_read < session->block_size && !end_of_file(session, bytes_read)) {
        session->incomplete_read = true;
        return false;
    }
    const size_t index = ((uint16_t) (session->next_data_packet_to_send - 1)) % session->window_size;
    const size_t offset = index * (sizeof(struct tftp_data_packet) + session->block_size);
    struct tftp_data_packet *packet = (void *) ((uint8_t *) session->data_packets + offset);
    tftp_data_packet_init(packet, session->next_data_packet_to_send);
    return true;
}

static bool set_max_retransmissions_error(struct tftp_session session[static 1]) {
    static const char format[] = "timeout after %d retransmits.";
    static const char format_miss_last_ack[] = "timeout after %d retransmits. Missed last ack.";
    const char *fmt = session->last_packet != -1 ? format_miss_last_ack : format;
    char *error_msg = malloc(sizeof(format_miss_last_ack) + 3 * sizeof(session->current_retransmission) + 2);
    if (error_msg == nullptr) {
        logger_log_error(session->logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    sprintf(error_msg, fmt, session->current_retransmission);
    session->stats.error = (struct tftp_session_stats_error) {
        .error_occurred = true,
        .error_number = TFTP_ERROR_NOT_DEFINED,
        .error_message = error_msg,
    };
    return true;
}

static bool report_client_error(struct tftp_session session[static 1], size_t error_packet_size) {
    struct tftp_error_packet *error_packet = (struct tftp_error_packet *) session->connection.recv_buffer;
    uint16_t error_code = ntohs(error_packet->error_code);
    char *error_message;
    const char *end_ptr = memchr(error_packet->error_message, '\0', error_packet_size - sizeof *error_packet);
    if (end_ptr == nullptr) {
        error_message = "Error message received was invalid (not null terminated).";
    }
    else {
        size_t error_message_len = end_ptr - (char *) error_packet->error_message;
        error_message = malloc(error_message_len + 1);
        if (error_message == nullptr) {
            logger_log_error(session->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        strcpy(error_message, (const char *) error_packet->error_message);
    }
    session->stats.error = (struct tftp_session_stats_error) {
        .error_occurred = true,
        .error_number = error_code,
        .error_message = error_message,
    };
    logger_log_warn(session->logger, "Error reported from client: %s", session->stats.error.error_message);
    return true;
}

static bool end_of_file(struct tftp_session session[static 1], size_t bytes_read) {
    off_t cur_offset = lseek(session->file_descriptor, 0, SEEK_CUR);
    bool is_file_seekable = (cur_offset != -1);
    if (!is_file_seekable) {
        return bytes_read == 0;
    }
    off_t end_offset = lseek(session->file_descriptor, 0, SEEK_END);
    lseek(session->file_descriptor, cur_offset, SEEK_SET);
    return cur_offset == end_offset;
}
