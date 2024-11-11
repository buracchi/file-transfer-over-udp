#include <buracchi/tftp/server_session.h>

#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <buracchi/tftp/dispatcher.h>
#include <logger.h>
#include <buracchi/tftp/server_session_handler.h>

enum event : uint32_t {
    EVENT_START,
    EVENT_TERMINATE,
    EVENT_OACK_SENT,
    EVENT_DATA_SENT,
    EVENT_ERROR_SENT,
    EVENT_NEXT_BLOCK_READY,
    EVENT_NEW_DATA,
    EVENT_TIMEOUT,
    EVENT_TIMEOUT_REMOVED,
};

static bool submit_send_error(struct tftp_session session[static 1]);
static bool submit_send_oack(struct tftp_session session[static 1]);
static bool submit_send_data(struct tftp_session session[static 1]);
static bool submit_fetch_next_block(struct tftp_session session[static 1]);
static bool submit_wait_new_data(struct tftp_session session[static 1]);
static bool submit_cancel_timeout(struct tftp_session session[static 1]);

static void close_session(struct tftp_session session[static 1]);

static bool on_start(struct tftp_session session[static 1]);

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
        .event_start = {.id = ((uint64_t) session_id << 48) | EVENT_START},
        .event_end = {.id = ((uint64_t) session_id << 48) | EVENT_TERMINATE},
        .event_timeout = {.event = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT}},
        .event_cancel_timeout = {.id = ((uint64_t) session_id << 48) | EVENT_TIMEOUT_REMOVED},
        .event_new_data = {.id = ((uint64_t) session_id << 48) | EVENT_NEW_DATA},
        .event_next_block = {.id = ((uint64_t) session_id << 48) | EVENT_NEXT_BLOCK_READY},
        .event_transmit_data = {.id = ((uint64_t) session_id << 48) | EVENT_DATA_SENT},
        .event_transmit_oack = {.id = ((uint64_t) session_id << 48) | EVENT_OACK_SENT},
        .event_transmit_error = {.id = ((uint64_t) session_id << 48) | EVENT_ERROR_SENT},
    };
}

enum tftp_session_state tftp_session_on_event(struct tftp_session session[static 1], struct dispatcher_event event[static 1]) {
    enum event e = event->id & 0xFFFF;
    enum tftp_handler_action next_action;
    if (e != EVENT_START && e != EVENT_TERMINATE) {
        session->pending_jobs--;
    }
    switch (e) {
        case EVENT_START:
            if (!on_start(session)) {
                close_session(session);
                return TFTP_SESSION_CLOSED;
            }
            session->event_timeout.timeout.tv_sec = session->handler.timeout;
            next_action = tftp_handler_start(&session->handler);
            break;
        case EVENT_TERMINATE:
            close_session(session);
            return TFTP_SESSION_CLOSED;
        case EVENT_ERROR_SENT: {
            next_action = tftp_handler_on_error_sent(&session->handler, event);
            break;
        }
        case EVENT_OACK_SENT: {
            next_action = tftp_handler_on_oack_sent(&session->handler, event);
            break;
        }
        case EVENT_DATA_SENT: {
            next_action = tftp_handler_on_data_sent(&session->handler, event);
            break;
        }
        case EVENT_NEXT_BLOCK_READY: {
            next_action = tftp_handler_on_next_block_ready(&session->handler, event);
            break;
        }
        case EVENT_NEW_DATA: {
            next_action = tftp_handler_on_new_data(&session->handler, event);
            break;
        }
        case EVENT_TIMEOUT: {
            next_action = tftp_handler_on_timeout(&session->handler, event);
            break;
        }
        case EVENT_TIMEOUT_REMOVED: {
            next_action = tftp_handler_on_timeout_removed(&session->handler, event);
            break;
        }
        default:
            logger_log_error(session->logger, "Unknown event id: %lu", e);
            exit(1);
            break;
    }
    switch (next_action) {
        case TFTP_HANDLER_ACTION_NONE:
            break;
        case TFTP_HANDLER_ACTION_SEND_ERROR:
            if (!submit_send_error(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_SEND_OACK:
            if (!submit_send_oack(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_FETCH_NEXT_BLOCK:
            if (!submit_fetch_next_block(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_SEND_DATA:
            if (!submit_send_data(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_WAIT_NEW_DATA:
            if (!submit_wait_new_data(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_CANCEL_TIMEOUT:
            if (!submit_cancel_timeout(session)) {
                exit(1);
            }
            break;
        case TFTP_HANDLER_ACTION_UNEXPECTED:
            exit(1);
            break;
    }
    if (session->handler.should_stop && session->pending_jobs == 0) {
        tftp_handler_destroy(&session->handler);
        close_session(session);
        return TFTP_SESSION_CLOSED;
    }
    return TFTP_SESSION_IDLE;
}


static void close_session(struct tftp_session session[static 1]) {
    session->is_active = false;
    session->handler = (struct tftp_handler) {};
    session->request_args = (struct tftp_peer_message) {};
}

static bool on_start(struct tftp_session session[static 1]) {
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
    option_ptr = (const char *) &session->request_args.buffer[2];
    if (token_length < 2 || token_length % 2 != 0) {
        logger_log_warn(session->logger, "Received ill formed packet, ignoring (tokens length: %zu).", token_length);
        return false;
    }
    bool ret = tftp_handler_init(&session->handler,
                                 session->server_info->server_addrinfo,
                                 session->request_args.peer_addr,
                                 session->request_args.peer_addrlen,
                                 session->request_args.is_orig_dest_addr_ipv4,
                                 session->server_info->root,
                                 option_size,
                                 option_ptr,
                                 session->server_info->retries,
                                 session->server_info->timeout,
                                 session->server_info->handler_stats_callback,
                                 session->logger);
    if (!ret) {
        logger_log_warn(session->logger, "Could not initialize handler.");
        return false;
    }
    if (mtx_lock(&session->server_info->server_stats->mtx) == thrd_error) {
        return false;
    }
    session->server_info->server_stats->counters.sessions_count++;
    if (mtx_unlock(&session->server_info->server_stats->mtx) == thrd_error) {
        return false;
    }
    return true;
}

static bool submit_send_error(struct tftp_session session[static 1]) {
    ssize_t error_packet_len = error_packet_init(&session->handler);
    if (error_packet_len == -1) {
        logger_log_error(session->logger, "Could not initialize error packet.");
        return false;
    }
    bool ret = dispatcher_submit_sendto(session->dispatcher,
                                        &session->event_transmit_error,
                                        session->handler.listener.file_descriptor,
                                        session->handler.error_packet,
                                        error_packet_len,
                                        0,
                                        (struct sockaddr *) &session->handler.listener.peer_addr_storage,
                                        (&session->handler)->listener.peer_addrlen);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting transmit error request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool submit_send_oack(struct tftp_session session[static 1]) {
    ssize_t packet_len = oack_packet_init(&session->handler);
    if (packet_len == -1) {
        logger_log_error(session->logger, "Could not initialize OACK packet.");
        return false;
    }
    bool ret = dispatcher_submit_sendto(session->dispatcher,
                                        &session->event_transmit_oack,
                                        session->handler.listener.file_descriptor,
                                        session->handler.oack_packet,
                                        packet_len,
                                        0,
                                        (struct sockaddr *) &session->handler.listener.peer_addr_storage,
                                        (&session->handler)->listener.peer_addrlen);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting transmit OACK request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool submit_send_data(struct tftp_session session[static 1]) {
    size_t next_data_packet_index = (session->handler.next_data_packet_to_send - 1) % session->handler.window_size;
    size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + session->handler.block_size);
    struct tftp_data_packet *packet = (struct tftp_data_packet *) ((uint8_t *) session->handler.data_packets + offset);
    bool is_last_packet = session->handler.next_data_packet_to_send == session->handler.next_data_packet_to_make - 1;
    size_t packet_size = sizeof *session->handler.data_packets + (is_last_packet ? session->handler.last_block_size : session->handler.block_size);
    bool ret = dispatcher_submit_sendto(session->dispatcher,
                                        &session->event_transmit_data,
                                        session->handler.listener.file_descriptor,
                                        packet,
                                        packet_size,
                                        0,
                                        (struct sockaddr *) &session->handler.listener.peer_addr_storage,
                                        session->handler.listener.peer_addrlen);
    if (!ret) {
        logger_log_error(session->logger, "Error while submitting transmit data request.");
        return false;
    }
    session->pending_jobs++;
    return true;
}

static bool submit_fetch_next_block(struct tftp_session session[static 1]) {
    size_t next_data_packet_to_make = session->handler.next_data_packet_to_make;
    size_t window_begin = session->handler.window_begin;
    size_t available_packets = session->handler.window_size - (next_data_packet_to_make - window_begin);
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

static bool submit_wait_new_data(struct tftp_session session[static 1]) {
    listener_setup(&session->handler);
    bool ret = dispatcher_submit_recvmsg_with_timeout(session->dispatcher,
                                                      &session->event_new_data,
                                                      &session->event_timeout,
                                                      session->handler.listener.file_descriptor,
                                                      &session->handler.listener.msghdr,
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
