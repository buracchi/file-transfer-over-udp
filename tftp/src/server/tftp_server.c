#include <buracchi/tftp/server.h>

#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include <buracchi/tftp/dispatcher.h>
#include <buracchi/tftp/server_stats.h>
#include <buracchi/tftp/server_thread_pool.h>
#include <buracchi/tftp/server_session.h>

#include <logger.h>

#include "../utils/utils.h"

static bool main_loop(struct tftp_server server[static 1], struct dispatcher dispatcher[static 1]);

static bool parse_request_metadata(struct tftp_peer_message request_args[static 1],
                                   struct msghdr msghdr[static 1],
                                   struct logger logger[static 1]);

bool tftp_server_init(struct tftp_server server[static 1], struct tftp_server_arguments args, struct logger logger[static 1]) {
    *server = (struct tftp_server) {
        .logger = logger,
        .root = args.root,
        .retries = args.retries,
        .timeout = args.timeout_s,
    };
    if (!tftp_server_stats_init(&server->stats, args.stats_interval_seconds, args.server_stats_callback, logger)) {
        logger_log_error(logger, "Failed to initialize server statistics. %s", strerror_rbs(errno));
        return false;
    }
    if (!tftp_server_listener_init(&server->listener, args.ip, args.port, logger)) {
        return false;
    }
    if (!tftp_server_thread_pool_init(&server->thread_pool,
                                      args.workers,
                                      args.max_worker_sessions,
                                      &(struct tftp_server_info) {
                                          .server_stats = &server->stats,
                                          .server_addrinfo = &server->listener.addrinfo,
                                          .timeout = server->timeout,
                                          .retries = server->retries,
                                          .root = server->root,
                                          .handler_stats_callback = args.handler_stats_callback,
                                      },
                                      server->logger)) {
        logger_log_error(server->logger, "Failed to initialize thread pool. %s", strerror_rbs(errno));
        return false;
    }
    return true;
}

bool tftp_server_start(struct tftp_server server[static 1]) {
    struct dispatcher dispatcher;
    if (!dispatcher_init(&dispatcher, 4, server->logger)) {
        return false;
    }
    logger_log_info(server->logger, "Server starting.");
    if (!main_loop(server, &dispatcher)) {
        dispatcher_destroy(&dispatcher);
        return false;
    }
    dispatcher_destroy(&dispatcher);
    logger_log_info(server->logger, "Server stopped.");
    return true;
}

void tftp_server_destroy(struct tftp_server server[static 1]) {
    logger_log_info(server->logger, "Server shutting down.");
    tftp_server_thread_pool_destroy(&server->thread_pool);
    tftp_server_listener_destroy(&server->listener);
    tftp_server_stats_destroy(&server->stats);
}

static bool main_loop(struct tftp_server server[static 1], struct dispatcher dispatcher[static 1]) {
    enum event_id : uint64_t { DATA_AVAILABLE, TIMEOUT };
    bool cancel_requests_dispatched = false;
    struct dispatcher_event on_data_available = {.id = DATA_AVAILABLE};
    struct dispatcher_event_timeout on_timeout = {.event = {.id = TIMEOUT}};
    struct tftp_session *session;
    if (server->stats.metrics_callback == nullptr) {
        logger_log_warn(server->logger, "No callback specified for server statistics logging, will continue without.");
    } else {
        if (!dispatcher_submit_timeout(dispatcher, &on_timeout, server->stats.interval)) {
            return false;
        }
    }
    session = tftp_server_thread_pool_new_session(&server->thread_pool);
    tftp_server_listener_set_recvmsg_dest(&server->listener, &session->request_args);
    if (!dispatcher_submit_recvmsg(dispatcher, &on_data_available, server->listener.file_descriptor, &server->listener.msghdr, MSG_TRUNC)) {
        dispatcher_submit(session->dispatcher, &session->event_end);
        return false;
    }
    while (!server->should_stop || dispatcher->pending_requests != 0) {
        struct dispatcher_event *event;
        errno = 0;
        if (!dispatcher_wait_event(dispatcher, &event)) {
            if (errno != EINTR) {
                logger_log_error(dispatcher->logger, "Server dispatcher failed: %s.", strerror_rbs(errno));
                return false;
            }
            if (server->should_stop && !cancel_requests_dispatched) {
                if (!dispatcher_submit_cancel_timeout(dispatcher, nullptr, &on_timeout)) {
                    return false;
                }
                if (!dispatcher_submit_cancel(dispatcher, nullptr, &on_data_available)) {
                    return false;
                }
                cancel_requests_dispatched = true;
                continue;
            }
        }
        if (event == nullptr) {
            continue;
        }
        switch (event->id) {
            case DATA_AVAILABLE:
                if (!event->is_success && event->error_number == ECANCELED) {
                    logger_log_debug(server->logger, "Server on data available request canceled.");
                    dispatcher_submit(session->dispatcher, &session->event_end);
                    continue;
                }
                if (!event->is_success) {
                    logger_log_error(server->logger, "Async recvmsg operation failed. %s", strerror_rbs(event->error_number));
                    dispatcher_submit(session->dispatcher, &session->event_end);
                }
                else {
                    ssize_t bytes_recvd = event->result;
                    session->request_args.bytes_recvd = bytes_recvd;
                    if (!parse_request_metadata(&session->request_args, &server->listener.msghdr, server->logger)) {
                        dispatcher_submit(session->dispatcher, &session->event_end);
                    }
                    else {
                        dispatcher_submit(session->dispatcher, &session->event_start);
                    }
                }
                session = tftp_server_thread_pool_new_session(&server->thread_pool);
                tftp_server_listener_set_recvmsg_dest(&server->listener, &session->request_args);
                if (!dispatcher_submit_recvmsg(dispatcher, &on_data_available, server->listener.file_descriptor, &server->listener.msghdr, MSG_TRUNC)) {
                    return false;
                }
                break;
            case TIMEOUT:
                if (!event->is_success && (event->error_number == ECANCELED)) {
                    logger_log_debug(server->logger, "Server metrics callback request canceled.");
                    break;
                }
                else if (!event->is_success && event->error_number != ETIME) {
                    logger_log_error(server->logger, "Timeout operation failed: %s", strerror_rbs(event->error_number));
                    return false;
                }
                logger_log_debug(server->logger, "Running the metrics callback.");
                server->stats.metrics_callback(&server->stats);
                if (!dispatcher_submit_timeout(dispatcher, &on_timeout, server->stats.interval)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

static bool parse_request_metadata(struct tftp_peer_message request_args[static 1],
                                   struct msghdr msghdr[static 1],
                                   struct logger logger[static 1]) {
    if (msghdr->msg_flags & MSG_TRUNC) {
        logger_log_warn(logger, "Received size greater than maximum request size, ignoring request.");
        return false;
    }
    if (msghdr->msg_flags & MSG_CTRUNC) {
        logger_log_warn(logger, "Received size greater than maximum ancillary buffer size allowed, ignoring request.");
        return false;
    }
    switch (request_args->peer_addr.ss_family) {
        case AF_INET:
            request_args->peer_addrlen = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            request_args->peer_addrlen = sizeof(struct sockaddr_in6);
            break;
        default:
            logger_log_warn(logger, "Received request from unsupported address family, ignoring request.");
            return false;
    }
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(msghdr); cmsg != nullptr; cmsg = CMSG_NXTHDR(msghdr, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_ORIGDSTADDR) {
            request_args->is_orig_dest_addr_ipv4 = true;
            break;
        }
    }
    return true;
}
