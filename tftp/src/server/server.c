#include <buracchi/tftp/server.h>

#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <buracchi/tftp/server_stats.h>

#include <logger.h>
#include <stdlib.h>

#include "session.h"
#include "worker_pool.h"
#include "../utils/time.h"
#include "../utils/utils.h"

static bool parse_request_metadata(struct tftp_peer_message request_args[static 1],
                                   struct msghdr msghdr[static 1],
                                   struct logger logger[static 1]);

bool tftp_server_init(struct tftp_server server[static 1], struct tftp_server_arguments args, struct logger logger[static 1]) {
    *server = (struct tftp_server) {
        .logger = logger,
        .root = args.root,
        .retries = args.retries,
        .timeout = args.timeout_s,
        .is_adaptive_timeout_enabled = args.is_adaptive_timeout_enabled,
        .is_write_request_enabled = args.is_write_request_enabled,
        .is_list_request_enabled = args.is_list_request_enabled,
        .worker_pool = malloc(sizeof *server->worker_pool),
        .session_stats_callback = args.session_stats_callback,
    };
    if (server->worker_pool == nullptr) {
        logger_log_error(logger, "Failed to initialize server. Could not allocate memory for the worker pool. %s", strerror_rbs(errno));
        return false;
    }
    if (!tftp_server_stats_init(&server->stats, args.stats_interval_seconds, args.server_stats_callback, logger)) {
        logger_log_error(logger, "Failed to initialize server statistics. %s", strerror_rbs(errno));
        return false;
    }
    if (!tftp_server_listener_init(&server->listener, args.ip, args.port, logger)) {
        return false;
    }
    if (!worker_pool_init(server->worker_pool,
                                      args.workers,
                                      args.max_worker_sessions,
                                      server->logger)) {
        logger_log_error(server->logger, "Failed to initialize thread pool. %s", strerror_rbs(errno));
        return false;
    }
    return true;
}

bool tftp_server_start(struct tftp_server server[static 1]) {
    struct tftp_server_info info = {
        .server_stats = &server->stats,
        .server_addrinfo = &server->listener.addrinfo,
        .timeout = server->timeout,
        .retries = server->retries,
        .root = server->root,
        .is_adaptive_timeout_enabled = server->is_adaptive_timeout_enabled,
        .is_write_request_enabled = server->is_write_request_enabled,
        .is_list_request_enabled = server->is_list_request_enabled,
        .session_stats_callback = server->session_stats_callback,
    };
    const bool metrics_enabled = server->stats.metrics_callback != nullptr;
    if (!metrics_enabled) {
        logger_log_warn(server->logger, "No callback specified for server statistics logging, will continue without.");
    }
    logger_log_info(server->logger, "Server starting.");
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    while (!server->should_stop) {
        struct worker_job *job = worker_pool_get_job(server->worker_pool);
        tftp_session_init(&job->session, job->job_id, &info, job->dispatcher, server->logger);
        job->session.request_args = (struct tftp_peer_message) {
            .peer_addrlen = sizeof job->session.request_args.peer_addr,
        };
        ssize_t *bytes_recvd = &job->session.request_args.bytes_recvd;
        memset(server->listener.msghdr.msg_control, 0, msg_control_size);
        server->listener.msghdr.msg_controllen = msg_control_size;
        server->listener.msghdr.msg_flags = 0;
        server->listener.msghdr.msg_name = &job->session.request_args.peer_addr;
        server->listener.msghdr.msg_namelen = job->session.request_args.peer_addrlen;
        server->listener.msghdr.msg_iov[0].iov_base = job->session.request_args.buffer;
        server->listener.msghdr.msg_iov[0].iov_len = sizeof job->session.request_args.buffer;
        
        bool is_timeout = false;
        if (metrics_enabled) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double remaining_time = server->stats.interval - (timespec_to_double(now) - timespec_to_double(start_time));
            if (remaining_time <= 0) {
                is_timeout = true;
            }
            else {
                const struct timeval timeout = double_to_timeval(remaining_time);
                if (setsockopt(server->listener.file_descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    logger_log_error(server->logger, "Failed to set socket receive timeout. %s", strerror_rbs(errno));
                    return false;
                }
            }
        }
        if (!is_timeout) {
            *bytes_recvd = recvmsg(server->listener.file_descriptor, &server->listener.msghdr, MSG_TRUNC);
        }
        enum revc_result {
            SUCCESS, TIMEOUT, INTERUPT, ERROR
        } result = *bytes_recvd >= 0 ? SUCCESS :
                   is_timeout || (metrics_enabled && (errno == EAGAIN || errno == EWOULDBLOCK)) ? TIMEOUT :
                   (errno == EINTR) ? INTERUPT :
                   ERROR;
        
        switch (result) {
            case SUCCESS:
                if (parse_request_metadata(&job->session.request_args, &server->listener.msghdr, server->logger)) {
                    int mtx_ret;
                    while ((mtx_ret = mtx_lock(&server->stats.mtx)) == thrd_error && errno == EINTR);
                    if (mtx_ret == thrd_error) {
                        logger_log_error(server->logger, "Failed to lock server stats mutex: %s", strerror_rbs(errno));
                        server->should_stop = true;
                        break;
                    }
                    if (worker_pool_start_job(server->worker_pool, job)) {
                        server->stats.counters.sessions_count++;
                    }
                    else {
                        logger_log_error(server->logger, "Failed to start session: %s", strerror_rbs(errno));
                    }
                    while ((mtx_ret = mtx_unlock(&server->stats.mtx)) == thrd_error && errno == EINTR);
                    if (mtx_ret == thrd_error) {
                        logger_log_error(server->logger, "Failed to unlock server stats mutex: %s", strerror_rbs(errno));
                        server->should_stop = true;
                    }
                }
                break;
            case TIMEOUT:
                logger_log_debug(server->logger, "Running the metrics callback.");
                server->stats.metrics_callback(&server->stats);
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                break;
            case INTERUPT:
                logger_log_trace(server->logger, "Received interrupt signal.");
                break;
            case ERROR:
                logger_log_error(server->logger, "Failed to receive message. %s", strerror_rbs(errno));
                return false;
        }
    }
    logger_log_info(server->logger, "Server stopped listening for requests.");
    return true;
}

void tftp_server_destroy(struct tftp_server server[static 1]) {
    logger_log_info(server->logger, "Awaiting for active sessions termination...");
    worker_pool_destroy(server->worker_pool);
    free(server->worker_pool);
    tftp_server_listener_destroy(&server->listener);
    tftp_server_stats_destroy(&server->stats);
    logger_log_info(server->logger, "Server shut down.");
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
