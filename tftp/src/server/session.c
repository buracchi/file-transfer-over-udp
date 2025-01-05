#include "session.h"

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <logger.h>

#include "dispatcher.h"
#include "session_handler.h"
#include "../utils/inet.h"

#define COROUTINE_START(state_ptr)      \
    int *coroutine_state_ = state_ptr;  \
    switch(*coroutine_state_) {         \
case 0:

#define AWAIT(async_function) do {      \
    *coroutine_state_ = __LINE__;       \
    return async_function;              \
case __LINE__:;                         \
    } while(0)

#define COROUTINE_END }

enum event : uint32_t {
    EVENT_START,
    EVENT_PACKET_SENT,
    EVENT_NEXT_BLOCK_READY,
    EVENT_PACKET_RECEIVED,
    EVENT_TIMEOUT,
    EVENT_TIMEOUT_REMOVED,
};

static bool fetch_next_block_async(struct tftp_session session[static 1]);
static bool recv_async(struct tftp_session session[static 1]);
static bool submit_cancel_timeout(struct tftp_session session[static 1]);
static bool send_packet_to_async(struct tftp_session session[static 1], const void *packet, size_t packet_size, const struct sockaddr addr[static 1], socklen_t addrlen);

static bool start(struct tftp_session session[static 1]);
static bool run(struct tftp_session session[static 1], struct dispatcher_event event[static 1]);
static void close_session(struct tftp_session session[static 1]);

static bool is_request_valid(struct tftp_session session[static 1]);
static bool oack_packet_init(struct tftp_session session[static 1]);
static bool error_packet_init(struct tftp_session session[static 1]);
static void fetch_next_block_netascii(struct tftp_session session[static 1]);
static void create_data_packets(struct tftp_session session[static 1], size_t bytes_read);
static bool report_client_error(struct tftp_session session[static 1], size_t error_packet_size);
static bool set_timeout_error(struct tftp_session session[static 1]);
static void set_unexpected_sender_error(struct tftp_session session[static 1]);

static bool end_of_file(struct tftp_handler handler[static 1]);

static inline bool is_block_in_window(uint16_t block, uint16_t window_begin, uint16_t window_size) {
    uint16_t window_end = window_begin + window_size - 1;
    if (window_begin <= window_end) {
        return block >= window_begin && block <= window_end;
    }
    return block >= window_begin || block <= window_end;
}

static inline size_t get_cumulative_ackd_payload_size(struct tftp_session session[static 1],
                                                      uint16_t block_number,
                                                      uint16_t last_packet_sent) {
    return session->handler.block_size * (block_number - session->handler.window_begin) +
           (block_number == last_packet_sent ? session->handler.last_block_size
                                             : session->handler.block_size);
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
        .session_id = session_id,
        .server_info = server_info,
        .dispatcher = dispatcher,
        .logger = logger,
        .connection = { .sockfd = -1, },
        .event_start = {.id = ((uint64_t) session_id << 48) | EVENT_START},
        .event_timeout = {.event = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT}},
        .event_cancel_timeout = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT_REMOVED},
        .event_new_data = {.id = ((uint64_t) session_id << 48) | EVENT_PACKET_RECEIVED},
        .event_next_block = {.id = ((uint64_t) session_id << 48) | EVENT_NEXT_BLOCK_READY},
        .event_packet_sent = {.id = ((uint64_t) session_id << 48) | EVENT_PACKET_SENT},
        .is_first_send = true,
        .oack_packet = nullptr,
        .error_packet = nullptr,
    };
}

enum tftp_session_state tftp_session_handle_event(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    enum event e = get_event(event);
    bool is_event_canceled = false;
    switch (e) {
        case EVENT_START:
            if (!start(session)) {
                return TFTP_SESSION_STATE_ERROR;
            }
            if (session->should_close) {
                is_event_canceled = true;
            }
            break;
        case EVENT_TIMEOUT:
            session->pending_jobs--;
            if (!event->is_success && (event->error_number == ENOENT || event->error_number == ECANCELED)) {
                is_event_canceled = true;
                break;
            }
            break;
        case EVENT_TIMEOUT_REMOVED:
            session->pending_jobs--;
            if (event->is_success) {
                is_event_canceled = true;
                break;
            }
            switch (event->error_number) {
                case 0: [[fallthrough]];
                case ENOENT:
                    is_event_canceled = true;
                    break;
                default:
                    logger_log_error(session->logger, "Error while removing timeout: %s", strerror(event->error_number));
                    return TFTP_SESSION_STATE_ERROR;
            }
            break;
        case EVENT_PACKET_RECEIVED:
            session->pending_jobs--;
            if (!event->is_success && event->error_number == ECANCELED) {
                is_event_canceled = true;
                break;
            }
            break;
        case EVENT_PACKET_SENT:
            session->pending_jobs--;
            if (event->is_success) {
                session->stats.packets_sent += 1;
            }
            break;
        case EVENT_NEXT_BLOCK_READY:
            session->pending_jobs--;
            break;
        default:
            logger_log_error(session->logger, "Unknown event id: %lu", e);
            return TFTP_SESSION_STATE_ERROR;
    }
    if (!is_event_canceled) {
        if (!run(session, event)) {
            return TFTP_SESSION_STATE_ERROR;
        }
    }
    if (session->should_close && session->pending_jobs == 0) {
        if (session->is_handler_instantiated) {
            tftp_handler_destroy(&session->handler);
        }
        close_session(session);
        return TFTP_SESSION_STATE_CLOSED;
    }
    return TFTP_SESSION_STATE_IDLE;
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
    
    const char *path = (char *)&session->request_args.buffer[2];
    if (!session_connection_init(&session->connection,
                                 session->server_info->server_addrinfo,
                                 session->request_args.peer_addr,
                                 session->request_args.peer_addrlen,
                                 session->request_args.is_orig_dest_addr_ipv4,
                                 session->logger)) {
        logger_log_error(session->logger, "Could not initialize session connection socket. Ignoring request.");
        return false;
    }
    
    if (session->connection.client_address.str != nullptr) {
        logger_log_info(session->logger, "New incoming connection from client '%s:%d' asking for path '%s'", session->connection.client_address.str, session->connection.client_address.port, path);
    }
    session->stats = tftp_session_stat_init(session->connection.address.sockaddr,
                                            session->connection.client_address.sockaddr,
                                            path,
                                            session->server_info->handler_stats_callback,
                                            session->logger);
    
    bool ret = tftp_handler_init(&session->handler,
                                 session->server_info->root,
                                 session->request_args.bytes_recvd - 2,
                                 (const char *) &session->request_args.buffer[2],
                                 session->server_info->retries,
                                 session->server_info->timeout,
                                 session->server_info->is_adaptive_timeout_enabled,
                                 &session->stats,
                                 session->logger);
    if (!ret) {
        logger_log_warn(session->logger, "Could not initialize handler.");
        return false;
    }
    session->is_handler_instantiated = true;
    if (session->handler.adaptive_timeout) {
        adaptive_timeout_init(&session->adaptive_timeout);
    }
    session->event_timeout.timeout.tv_sec = session->handler.timeout;
    return true;
}

static bool run(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    COROUTINE_START(&session->coroutine_state)
            while (true) {
main_loop:
                enum {
                    SEND_ERROR, SEND_OACK, SEND_DATA
                } next_action =
                    session->stats.error.error_occurred ? SEND_ERROR :
                    session->handler.valid_options_required && !session->handler.options_acknowledged ? SEND_OACK :
                    SEND_DATA;
                if (next_action == SEND_ERROR) {
                    if (!error_packet_init(session)) {
                        logger_log_error(session->logger, "Could not initialize error packet.");
                        return false;
                    }
                    AWAIT(send_packet_to_async(session, session->error_packet, session->error_packet_size,
                                               session->connection.last_message_address.sockaddr,
                                               session->connection.last_message_address.addrlen));
                    if (!event->is_success) {
                        logger_log_error(session->logger, "Error while sending ERROR: %s", strerror(event->error_number));
                        return false;
                    }
                    logger_log_trace(session->logger, "Sent ERROR <message=%s> to %s:%d", session->stats.error.error_message, session->connection.last_message_address.str, session->connection.last_message_address.port);
                    if (session->stats.error.is_non_terminating) {
                        session->stats.error.error_occurred = false;
                        continue;
                    }
                    session->should_close = true;
                    return true;
                }
                else if (next_action == SEND_OACK) {
                    if (get_event(event) != EVENT_TIMEOUT) {
                        if (!oack_packet_init(session)) {
                            logger_log_error(session->logger, "Could not initialize OACK packet.");
                            return false;
                        }
                        if (session->handler.adaptive_timeout) {
                            adaptive_timeout_start_timer(&session->adaptive_timeout);
                            session->adaptive_timeout.starting_block_number = 0;
                            session->adaptive_timeout.packets_sent = 1;
                        }
                    }
                    else {
                        adaptive_timeout_cancel_timer(&session->adaptive_timeout);
                    }
                    AWAIT(send_packet_to_async(session, session->oack_packet, session->oack_packet_size, session->connection.client_address.sockaddr, session->connection.client_address.addrlen));
                    if (!event->is_success) {
                        logger_log_error(session->logger, "Error while sending OACK: %s", strerror(event->error_number));
                        return false;
                    }
                    logger_log_trace(session->logger, "Sent OACK %s to %s:%d", session->stats.options_acked, session->connection.client_address.str, session->connection.client_address.port);
                }
                else if (next_action == SEND_DATA) {
                    if (session->is_first_send) {
                        session->handler.window_begin = 1;
                        session->is_first_send = false;
                    }
                    
                    if (get_event(event) == EVENT_TIMEOUT) {
                        session->handler.next_data_packet_to_send = session->handler.window_begin;
                        adaptive_timeout_cancel_timer(&session->adaptive_timeout);
                    }
                    else {  // create data packets
                        if (session->handler.mode == TFTP_MODE_OCTET) {
                            do {
                                AWAIT(fetch_next_block_async(session));
                                if (!event->is_success) {
                                    logger_log_warn(session->logger, "Error while reading from source: %s", strerror(event->error_number));
                                    session->stats.error = (struct tftp_session_stats_error) {
                                        .error_occurred = true,
                                        .error_number = TFTP_ERROR_NOT_DEFINED,
                                        .error_message = "Error while reading from source"
                                    };
                                    goto main_loop; // send ERROR
                                }
                                create_data_packets(session, event->result);
                            } while (session->handler.incomplete_read);
                        } else if (session->handler.mode == TFTP_MODE_NETASCII) {
                            fetch_next_block_netascii(session);
                        }
                    } // send data packets
                    bool is_last_packet = false;
                    bool is_packet_last_in_window = false;
                    if (session->handler.adaptive_timeout) {
                        if (!session->adaptive_timeout.is_timer_active) {
                            adaptive_timeout_start_timer(&session->adaptive_timeout);
                            session->adaptive_timeout.starting_block_number = session->handler.next_data_packet_to_send;
                            session->adaptive_timeout.packets_sent = 0;
                        }
                        session->adaptive_timeout.packets_sent += session->handler.next_data_packet_to_make - session->handler.next_data_packet_to_send;
                    }
                    do {
                        size_t next_data_packet_index = ((uint16_t) (session->handler.next_data_packet_to_send - 1)) % session->handler.window_size;
                        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + session->handler.block_size);
                        auto packet = (struct tftp_data_packet *) ((uint8_t *) session->handler.data_packets + offset);
                        is_last_packet = session->handler.next_data_packet_to_send == (uint16_t) (session->handler.next_data_packet_to_make - 1);
                        size_t packet_size = sizeof *session->handler.data_packets + (is_last_packet ? session->handler.last_block_size : session->handler.block_size);
                        AWAIT(send_packet_to_async(session, packet, packet_size, session->connection.client_address.sockaddr, session->connection.client_address.addrlen));
                        if (!event->is_success) {
                            logger_log_error(session->logger, "Error while sending data: %s", strerror(event->error_number));
                            return false;
                        }
                        next_data_packet_index = ((uint16_t) (session->handler.next_data_packet_to_send - 1)) % session->handler.window_size;
                        offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + session->handler.block_size);
                        packet = (struct tftp_data_packet *) ((uint8_t *) session->handler.data_packets + offset);
                        uint16_t block_number = ntohs(packet->block_number);
                        uint16_t sent_data_field_size = event->result - sizeof(struct tftp_data_packet);
                        logger_log_trace(session->logger, "Sent DATA <block=%d, size=%zu bytes> to %s:%d", block_number, sent_data_field_size, session->connection.client_address.str, session->connection.client_address.port);
                        session->handler.next_data_packet_to_send += 1;
                        is_last_packet = (session->handler.next_data_packet_to_send == session->handler.next_data_packet_to_make) &&
                                              (session->handler.last_block_size < session->handler.block_size);
                        if (is_last_packet) {
                            session->is_waiting_last_ack = true;
                        }
                        is_packet_last_in_window = ((uint16_t) (session->handler.next_data_packet_to_send - session->handler.window_size)) == session->handler.window_begin;
                    } while (!is_last_packet && !is_packet_last_in_window);
                }
                while (true) {  // a received packet may be ignored an arbitrary number of times
                    AWAIT(recv_async(session));
                    if (get_event(event) == EVENT_TIMEOUT) {
                        if (!event->is_success && event->error_number != ETIME) {
                            logger_log_error(session->logger, "Error while handling timeout: %s", strerror(event->error_number));
                            return false;
                        }
                        if (session->handler.retransmits < session->handler.retries) {
                            logger_log_debug(session->logger, "ACK timeout for client %s:%d. Retransmission no %d.", session->connection.client_address.str, session->connection.client_address.port, session->handler.retransmits + 1);
                            session->handler.retransmits += 1;
                            session->handler.global_retransmits += 1;
                            session->handler.timeout = true;
                            if (session->handler.adaptive_timeout) {
                                adaptive_timeout_backoff(&session->adaptive_timeout);
                                struct __kernel_timespec timeout = {.tv_sec = session->adaptive_timeout.rto.tv_sec, .tv_nsec = session->adaptive_timeout.rto.tv_nsec};
                                session->event_timeout.timeout = timeout;
                                logger_log_trace(session->logger, "Exponential backoff: adaptive timeout set to %lld.%.9ld seconds.", timeout.tv_sec, timeout.tv_nsec);
                            }
                            break; // retransmit
                        }
                        // max retransmits reached
                        if (!set_timeout_error(session)) {
                            return false;
                        }
                        break; // send ERROR
                    }
                    
                    if (!event->is_success) {
                        logger_log_error(session->logger, "Error while receiving data: %s", strerror(event->error_number));
                        return false;
                    }
                    // TODO: ret must be at least 4 bytes
                    bool is_address_family_valid = false;
                    switch (session->connection.last_message_address.storage.ss_family) {
                        case AF_INET:
                            session->connection.last_message_address.addrlen = sizeof(struct sockaddr_in);
                            is_address_family_valid = true;
                            break;
                        case AF_INET6:
                            session->connection.last_message_address.addrlen = sizeof(struct sockaddr_in6);
                            is_address_family_valid = true;
                            break;
                        default:
                            logger_log_warn(session->logger, "Unknown address family: %d. Ignoring packet.", session->connection.last_message_address.storage.ss_family);
                            break;
                    }
                    if (!is_address_family_valid) {
                        continue;  // ignore unexpected packet
                    }
                    session->connection.last_message_address.str = nullptr;
                    if (sockaddr_ntop(session->connection.last_message_address.sockaddr, session->connection.last_message_address.str_storage, &session->connection.last_message_address.port) != nullptr) {
                        session->connection.last_message_address.str = session->connection.last_message_address.str_storage;
                    }
                    if (!sockaddr_equals(&session->connection.last_message_address.storage, &session->connection.client_address.storage)) {
                        set_unexpected_sender_error(session);
                        break;   // send error packet
                    }
                    enum tftp_opcode opcode = ntohs(*(uint16_t *) session->connection.recv_buffer);
                    switch (opcode) {
                        case TFTP_OPCODE_ERROR:
                            session->should_close = true;
                            return report_client_error(session, event->result);
                        case TFTP_OPCODE_ACK:
                            uint16_t block_number = ntohs(*(uint16_t *) &session->connection.recv_buffer[2]);
                            uint16_t last_packet_sent = session->handler.next_data_packet_to_send - 1;
                            logger_log_trace(session->logger, "Received ACK <block=%d> from %s:%d", block_number, session->connection.client_address.str, session->connection.client_address.port);
                            if (!is_block_in_window(block_number, session->handler.window_begin, session->handler.window_size)) {
                                continue; // ignore unexpected ACK
                            }
                            submit_cancel_timeout(session);
                            if (session->handler.adaptive_timeout && session->adaptive_timeout.is_timer_active) {
                                const uint16_t window_end = session->adaptive_timeout.starting_block_number + session->adaptive_timeout.packets_sent - 1;
                                const uint16_t offset = block_number - session->adaptive_timeout.starting_block_number;
                                const uint16_t range = window_end - session->adaptive_timeout.starting_block_number;
                                if (offset <= range) {
                                    adaptive_timeout_stop_timer(&session->adaptive_timeout);
                                    struct __kernel_timespec timeout = {.tv_sec = session->adaptive_timeout.rto.tv_sec, .tv_nsec = session->adaptive_timeout.rto.tv_nsec};
                                    session->event_timeout.timeout = timeout;
                                    logger_log_trace(session->logger, "Adaptive timeout set to %lld.%.9ld seconds.", timeout.tv_sec, timeout.tv_nsec);
                                }
                            }
                            if (session->handler.valid_options_required && block_number == 0) {
                                session->handler.options_acknowledged = true;
                            }
                            else {
                                session->stats.bytes_sent += get_cumulative_ackd_payload_size(session, block_number, last_packet_sent);
                            }
                            session->handler.window_begin = block_number + 1;
                            session->stats.packets_acked += 1;
                            session->handler.retransmits = 0;
                            if (session->is_waiting_last_ack) {
                                if (block_number == last_packet_sent) {
                                    session->should_close = true;
                                    return true;
                                }
                                continue; // no more packets to send
                            }
                            break;
                        default:
                            logger_log_error(session->logger, "Expected an ACK opcode from %s:%d, got: %hu.", session->connection.client_address.str, session->connection.client_address.port, opcode);
                            session->stats.error = (struct tftp_session_stats_error) {
                                .error_occurred = true,
                                .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
                                .error_message = "I only do reads for now...",
                            };
                            break;
                    }
                    break;
                }
            }
    COROUTINE_END
    unreachable();
}

static void close_session(struct tftp_session session[static 1]) {
    if (session->connection.sockfd != -1) {
        session_connection_destroy(&session->connection, session->logger);
    }
    free(session->oack_packet);
    free(session->error_packet);
    logger_log_debug(session->logger, "Session closed.");
}

static bool is_request_valid(struct tftp_session session[static 1]) {
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(session->request_args.buffer);
    if (opcode != TFTP_OPCODE_RRQ) {
        logger_log_warn(session->logger, "Unexpected TFTP opcode %d, expected %d.", opcode, TFTP_OPCODE_RRQ);
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

static bool send_packet_to_async(struct tftp_session session[static 1], const void *packet, size_t packet_size, const struct sockaddr addr[static 1], socklen_t addrlen) {
    bool ret = dispatcher_submit_sendto(session->dispatcher,
                                        &session->event_packet_sent,
                                        session->connection.sockfd,
                                        packet,
                                        packet_size,
                                        0,
                                        addr,
                                        addrlen);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting send packet request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool fetch_next_block_async(struct tftp_session session[static 1]) {
    uint16_t block_diff = session->handler.next_data_packet_to_make - session->handler.window_begin;
    uint16_t available_packets = session->handler.window_size - block_diff;
    next_block_setup(&session->handler);
    if (!dispatcher_submit_readv(session->dispatcher,
                                 &session->event_next_block,
                                 session->handler.file_descriptor,
                                 session->handler.file_iovec,
                                 available_packets)) {
        logger_log_error(session->logger, "Could not submit readv request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool recv_async(struct tftp_session session[static 1]) {
    session->connection.last_message_address.addrlen = sizeof session->connection.last_message_address.storage;
    session->connection.iovec[0].iov_base = session->connection.recv_buffer;
    session->connection.iovec[0].iov_len = sizeof session->connection.recv_buffer;
    session->connection.msghdr.msg_name = &session->connection.last_message_address.storage;
    session->connection.msghdr.msg_namelen = session->connection.last_message_address.addrlen;
    bool ret = dispatcher_submit_recvmsg_with_timeout(session->dispatcher,
                                                      &session->event_new_data,
                                                      &session->event_timeout,
                                                      session->connection.sockfd,
                                                      &session->connection.msghdr,
                                                      0);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting receive new data request.");
        return false;
    }
    session->pending_jobs += 2;
    return true;
}

static bool submit_cancel_timeout(struct tftp_session session[static 1]) {
    bool ret = dispatcher_submit_cancel_timeout(session->dispatcher, &session->event_cancel_timeout, &session->event_timeout);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting cancel timeout request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool oack_packet_init(struct tftp_session session[static 1]) {
    size_t options_values_length = 0;
    for (enum tftp_option_recognized option = 0; option < TFTP_OPTION_TOTAL_OPTIONS; option++) {
        if (session->handler.options[option].is_active) {
            options_values_length += strlen(tftp_option_recognized_string[option]) + 1;
            options_values_length += strlen(session->handler.options[option].value) + 1;
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
        if (session->handler.options[option].is_active) {
            size_t len = strlen(tftp_option_recognized_string[option]) + 1;
            ptr = (unsigned char *) memcpy(ptr, tftp_option_recognized_string[option], len) + len;
            len = strlen(session->handler.options[option].value) + 1;
            ptr = (unsigned char *) memcpy(ptr, session->handler.options[option].value, len) + len;
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

/**
 * This is comically bad but I don't plan to improve this in the near future.
 */
static void fetch_next_block_netascii(struct tftp_session session[static 1]) {
    struct tftp_handler *handler = &session->handler;
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

static void create_data_packets(struct tftp_session session[static 1], size_t bytes_read) {
    struct tftp_handler *handler = &session->handler;
    uint16_t block_diff = handler->next_data_packet_to_make - handler->window_begin;
    uint16_t writable_blocks = handler->window_size - block_diff;
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
        size_t next_data_packet_index = ((uint16_t) (handler->next_data_packet_to_make - 1)) % handler->window_size;
        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
        struct tftp_data_packet *packet = (struct tftp_data_packet *) ((uint8_t *) handler->data_packets + offset);
        tftp_data_packet_init(packet, handler->next_data_packet_to_make);
        handler->next_data_packet_to_make += 1;
    }
    if (whole_blocks_read != writable_blocks && !end_of_file(handler)) {
        handler->incomplete_read = true;
    }
}

static bool set_timeout_error(struct tftp_session session[static 1]) {
    struct tftp_handler *handler = &session->handler;
    static const char format[] = "timeout after %d retransmits.";
    static const char format_miss_last_ack[] = "timeout after %d retransmits. Missed last ack.";
    char *error_msg = malloc(sizeof(format_miss_last_ack) + 3 * sizeof(handler->retransmits) + 2);
    if (error_msg == nullptr) {
        logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    sprintf(error_msg, session->is_waiting_last_ack ? format : format_miss_last_ack, handler->retransmits);
    session->stats.error.error_occurred = true;
    session->stats.error.error_number = TFTP_ERROR_NOT_DEFINED;
    session->stats.error.error_message = error_msg;
    return true;
}

static void set_unexpected_sender_error(struct tftp_session session[static 1]) {
    if (session->connection.last_message_address.str == nullptr) {
        logger_log_error(session->logger, "Unexpected sender, could not translate client address to a string representation.");
    }
    else {
        logger_log_warn(session->logger, "Unexpected sender: '%s:%d', expected client: '%s:%d'.", session->connection.last_message_address.str, session->connection.last_message_address.port, session->connection.client_address.str, session->connection.client_address.port);
    }
    session->stats.error = (struct tftp_session_stats_error) {
        .error_occurred = true,
        .is_non_terminating = true,
        .error_number = TFTP_ERROR_UNKNOWN_TRANSFER_ID,
        .error_message = "Unknown transfer ID.",
    };
}

static bool report_client_error(struct tftp_session session[static 1], size_t error_packet_size) {
    struct tftp_handler *handler = &session->handler;
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
            logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        strcpy(error_message, (const char *) error_packet->error_message);
    }
    session->stats.error = (struct tftp_session_stats_error) {
        .error_occurred = true,
        .error_number = error_code,
        .error_message = error_message,
    };
    logger_log_warn(handler->logger, "Error reported from client: %s", session->stats.error.error_message);
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
