#include "tftp_server.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include <errno.h>

#include "tftp.h"

#include "tftp_server_stats.h"
#include "tftp_handler.h"
#include "inet_utils.h"

constexpr size_t msg_controllen = 4096;

static bool listener_init(struct tftp_server server[static 1], const char *host, const char *service);

static bool submit_metrics_timeout_request(struct tftp_server server[static 1]);
static void remove_metrics_timeout_request(struct tftp_server server[static 1]);
static void on_metrics_timeout(int32_t ret, void *args);
static void on_metrics_timeout_request_removed(int32_t ret, void *args);

static void submit_on_data_available_request(struct tftp_server server[static 1]);
static void remove_on_data_available_request(struct tftp_server server[static 1]);
static void on_data_available(int32_t ret, void *args);
static void on_data_available_request_removed(int32_t ret, void *args);

static void on_new_tftp_request(int32_t ret, void *args);

bool tftp_server_init(struct tftp_server server[static 1], struct tftp_server_arguments args, struct logger logger[static 1]) {
    *server = (struct tftp_server) {
        .logger = logger,
        .should_stop = false,
        .thread_pool = nullptr,
        .is_timeout_job_pending = false,
        .timeout_job = {
            .routine = on_metrics_timeout,
            .args = server,
        },
        .remove_timeout_job = {
            .routine = on_metrics_timeout_request_removed,
            .args = server,
        },
        .is_on_data_available_job_pending = false,
        .request_context = {
            .peer_addrlen = sizeof server->request_context.peer_addr,
            .iovec = {
                {
                    .iov_base = server->request_context.buffer,
                    .iov_len = sizeof server->request_context.buffer
                }
            },
        },
        .data_available_job = {
            .routine = on_data_available,
            .args = server,
        },
        .remove_data_available_job = {
            .routine = on_data_available_request_removed,
            .args = server,
        },
        .server_stats_callback = args.server_stats_callback,
        .handler_stats_callback = args.handler_stats_callback,
        .listener = -1,
        .root = args.root,
        .retries = args.retries,
        .timeout = args.timeout_s,
    };
    if (!tftp_server_stats_init(&server->stats, args.stats_interval_seconds, logger)) {
        logger_log_error(logger, "Failed to initialize server statistics. %s", strerror(errno));
        return false;
    }
    server->stats_timeout_timespec.tv_sec = server->stats.interval;
    server->request_context.msghdr = (struct msghdr) {
            .msg_name = &server->request_context.peer_addr,
            .msg_namelen = server->request_context.peer_addrlen,
            .msg_iov = server->request_context.iovec,
            .msg_iovlen = 1,
            .msg_control = malloc(msg_controllen),
            .msg_controllen = msg_controllen,
            .msg_flags = MSG_CTRUNC
    };
    if (server->request_context.msghdr.msg_control == nullptr) {
        logger_log_error(logger, "Failed to initialize msghdr control buffer. %s", strerror(errno));
        return false;
    }
    server->thread_pool = malloc(sizeof *server->thread_pool + args.workers * sizeof *server->thread_pool->workers);
    if (server->thread_pool == nullptr) {
        logger_log_error(logger, "Failed to initialize thread pool. %s", strerror(errno));
        return false;
    }
    uring_thread_pool_init(server->thread_pool, args.workers, args.max_worker_sessions, logger);
    int ret = io_uring_queue_init(4, &server->ring, 0);
    if (ret < 0) {
        logger_log_error(logger, "Failed to initialize server io_uring %s.", -ret);
        return false;
    }
    if (!listener_init(server, args.ip, args.port)) {
        return false;
    }
    return true;
}

bool tftp_server_start(struct tftp_server server[static 1]) {
    bool pending_requests_removed = false;
    logger_log_info(server->logger, "Server starting.");
    if (!submit_metrics_timeout_request(server)) {
        return false;
    }
    submit_on_data_available_request(server);
    while (!(server->should_stop &&
             io_uring_cq_ready(&server->ring) == 0 &&
             io_uring_sq_ready(&server->ring) == 0 &&
             !server->is_on_data_available_job_pending &&
             !server->is_timeout_job_pending)) {
        struct io_uring_cqe *cqe;
        errno = 0;
        int ret = io_uring_wait_cqe(&server->ring, &cqe);
        if (ret != -EINTR) {
            if (ret < 0) {
                logger_log_error(server->logger, "io_uring_wait_cqe failed: %s.", strerror(-ret));
                exit(1);
            }
            struct uring_job *job = io_uring_cqe_get_data(cqe);
            if (job != nullptr) {
                job->routine(cqe->res, job->args);
            }
            io_uring_cqe_seen(&server->ring, cqe);
        }
        if (server->should_stop && !pending_requests_removed) {
            remove_metrics_timeout_request(server);
            remove_on_data_available_request(server);
            pending_requests_removed = true;
        }
    }
    logger_log_info(server->logger, "Server stopped.");
    return true;
}

void tftp_server_destroy(struct tftp_server server[static 1]) {
    logger_log_info(server->logger, "Server shutting down.");
    uring_thread_pool_destroy(server->thread_pool);
    free(server->thread_pool);
    io_uring_queue_exit(&server->ring);
    close(server->listener);
    free(server->request_context.msghdr.msg_control);
    tftp_server_stats_destroy(&server->stats);
}

static bool listener_init(struct tftp_server server[static 1], const char *host, const char *service) {
    struct addrinfo *addrinfo_list = nullptr;
    static const struct addrinfo hints = {
        .ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_ADDRCONFIG,
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = nullptr,
        .ai_canonname = nullptr,
        .ai_next = nullptr
    };
    int gai_ret = getaddrinfo(host, service, &hints, &addrinfo_list);
    if (gai_ret) {
        logger_log_error(server->logger, "Could not translate the host and service required to a valid internet address. %s", gai_strerror(gai_ret));
        if (addrinfo_list != nullptr) {
            freeaddrinfo(addrinfo_list);
        }
        return false;
    }
    for (struct addrinfo *addrinfo = addrinfo_list; addrinfo != nullptr; addrinfo = addrinfo->ai_next) {
        char bind_addr_str[INET6_ADDRSTRLEN] = {};
        uint16_t bind_addr_port;
        server->listener = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if (server->listener == -1) {
            logger_log_error(server->logger, "Could not create a socket for the server. %s", strerror(errno));
            goto fail;
        }
        if (setsockopt(server->listener, SOL_SOCKET, SO_REUSEADDR, &(int) {true}, sizeof(int)) == -1 ||
            setsockopt(server->listener, IPPROTO_IP, IP_RECVORIGDSTADDR, &(int) {true}, sizeof(int)) == -1 ||
            setsockopt(server->listener, IPPROTO_IPV6, IPV6_RECVORIGDSTADDR, &(int) {true}, sizeof(int)) == -1 ||
            setsockopt(server->listener, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {false}, sizeof(int)) == -1) {
            logger_log_error(server->logger, "Could not set listener socket options for the server. %s", strerror(errno));
            goto fail2;
        }
        if (inet_ntop_address(addrinfo->ai_addr, bind_addr_str, &bind_addr_port) == nullptr) {
            logger_log_error(server->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
            goto fail2;
        }
        logger_log_debug(server->logger, "Trying to bind to host %s on port %hu", bind_addr_str, bind_addr_port);
        if (bind(server->listener, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
            logger_log_debug(server->logger, "Can't bind to this address. %s", strerror(errno));
            close(server->listener);
            continue;
        }
        logger_log_info(server->logger, "Server is listening on %s port %hu", bind_addr_str, bind_addr_port);
        memcpy(&server->addr_storage, addrinfo->ai_addr, addrinfo->ai_addrlen);
        server->addrinfo = (struct addrinfo) {
            .ai_addr = (struct sockaddr *) &server->addr_storage,
            .ai_addrlen = addrinfo->ai_addrlen,
            .ai_family = addrinfo->ai_family,
            .ai_socktype = addrinfo->ai_socktype,
            .ai_flags = addrinfo->ai_flags,
            .ai_protocol = addrinfo->ai_protocol,
            .ai_next = nullptr,
            .ai_canonname = nullptr,
        };
        freeaddrinfo(addrinfo_list);
        return true;
    }
    logger_log_error(server->logger, "Failed to bind server to any address");
    if (server->listener == -1) {
        goto fail;
    }
fail2:
    close(server->listener);
fail:
    freeaddrinfo(addrinfo_list);
    return false;
}

static bool submit_metrics_timeout_request(struct tftp_server server[static 1]) {
    if (server->server_stats_callback == nullptr) {
        logger_log_warn(server->logger, "No callback specified for server statistics logging, will continue without.");
        return true;
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    if (sqe == nullptr) {
        logger_log_error(server->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_timeout(sqe, &server->stats_timeout_timespec, 0, 0);
    io_uring_sqe_set_data(sqe, &server->timeout_job);
    int ret = io_uring_submit(&server->ring);
    if (ret < 0) {
        logger_log_error(server->logger, "Could not submit the timeout request. %s", strerror(-ret));
        return false;
    }
    server->is_timeout_job_pending = true;
    logger_log_debug(server->logger, "Starting the metrics callback in %ds.", server->stats.interval);
    return true;
}

static void remove_metrics_timeout_request(struct tftp_server server[static 1]) {
    if (server->server_stats_callback == nullptr) {
        return;
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    if (sqe == nullptr) {
        logger_log_error(server->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_timeout_remove(sqe, (size_t) &server->timeout_job, 0);
    io_uring_sqe_set_data(sqe, &server->remove_timeout_job);
    int ret = io_uring_submit(&server->ring);
    if (ret < 0) {
        logger_log_error(server->logger, "Could not remove the metrics callback request. %s", strerror(-ret));
        exit(1);
    }
    logger_log_debug(server->logger, "Removing the metrics callback request.");
}

static void on_metrics_timeout(int32_t ret, void *args) {
    struct tftp_server *server = args;
    server->is_timeout_job_pending = false;
    if (ret < 0) {
        switch (-ret) {
            case ECANCELED:
                logger_log_debug(server->logger, "Server metrics callback request canceled.");
                return;
            case ETIME:
                break;
            default:
                logger_log_error(server->logger, "Timeout operation failed. %s.", strerror(-ret));
                exit(1);
        }
    }
    logger_log_debug(server->logger, "Running the metrics callback.");
    server->server_stats_callback(&server->stats);
    submit_metrics_timeout_request(server);
}

static void on_metrics_timeout_request_removed(int32_t ret, void *args) {
    struct tftp_server *server = args;
    server->is_timeout_job_pending = false;
}

static void submit_on_data_available_request(struct tftp_server server[static 1]) {
    server->request_context.msghdr.msg_controllen = msg_controllen;
    server->request_context.msghdr.msg_flags = MSG_CTRUNC;
    for (size_t i = 0; i < msg_controllen; i++) {
        ((char *)server->request_context.msghdr.msg_control)[i] = 0;
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    if (sqe == nullptr) {
        logger_log_error(server->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_recvmsg(sqe, server->listener, &server->request_context.msghdr, MSG_PEEK | MSG_TRUNC);
    io_uring_sqe_set_data(sqe, &server->data_available_job);
    int ret = io_uring_submit(&server->ring);
    if (ret < 0) {
        logger_log_error(server->logger, "Could not submit the on data available request. %s", strerror(-ret));
        exit(1);
    }
    server->is_on_data_available_job_pending = true;
}

static void remove_on_data_available_request(struct tftp_server server[static 1]) {
    if (!server->is_on_data_available_job_pending) {
        return;
    }
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    if (sqe == nullptr) {
        logger_log_error(server->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    io_uring_prep_cancel(sqe, &server->data_available_job, 0);
    io_uring_sqe_set_data(sqe, nullptr);
    int ret = io_uring_submit(&server->ring);
    if (ret < 0) {
        logger_log_error(server->logger, "Could not remove the on data available request. %s", strerror(-ret));
        exit(1);
    }
    logger_log_debug(server->logger, "Removing the server on data available request.");
}

static void on_data_available(int32_t ret, void *args) {
    struct tftp_server *server = args;
    bool is_orig_dest_addr_ipv4 = false;
    ssize_t bytes_recvd;
    server->is_on_data_available_job_pending = false;
    if (ret < 0) {
        if (-ret == ECANCELED) {
            logger_log_debug(server->logger, "Server on data available request canceled.");
            return;
        }
        logger_log_error(server->logger, "Async recvmsg operation failed. %s", strerror(-ret));
        exit(1);
    }
    if (server->request_context.msghdr.msg_flags & MSG_TRUNC) {
        logger_log_warn(server->logger, "Received size greater than maximum request size, ignoring request.");
        submit_on_data_available_request(server);
        return;
    }
    if (server->request_context.msghdr.msg_flags & MSG_CTRUNC) {
        logger_log_warn(server->logger, "Received size greater than maximum ancillary buffer size allowed, ignoring request.");
        submit_on_data_available_request(server);
        return;
    }
    switch (server->request_context.peer_addr.ss_family) {
        case AF_INET:
            server->request_context.peer_addrlen = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            server->request_context.peer_addrlen = sizeof(struct sockaddr_in6);
            break;
        default:
            logger_log_warn(server->logger, "Received request from unsupported address family, ignoring request.");
            submit_on_data_available_request(server);
            return;
    }
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&server->request_context.msghdr); cmsg != nullptr; cmsg = CMSG_NXTHDR(&server->request_context.msghdr, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_ORIGDSTADDR) {
            is_orig_dest_addr_ipv4 = true;
            break;
        }
    }
    bytes_recvd = recvmsg(server->listener, &server->request_context.msghdr, 0);
    if (bytes_recvd == -1) {
        logger_log_error(server->logger, "%s", strerror(errno));
        exit(1);
    }
    struct uring_worker_tftp_handler *worker_handler = uring_thread_pool_get_available_worker_handler(server->thread_pool);
    worker_handler->new_request_args = (struct new_tftp_request_args) {
            .server = server,
            .peer_addr = server->request_context.peer_addr,
            .peer_addrlen = server->request_context.peer_addrlen,
            .bytes_recvd = bytes_recvd,
            .is_orig_dest_addr_ipv4 = is_orig_dest_addr_ipv4,
    };
    memcpy(worker_handler->new_request_args.buffer, server->request_context.buffer, bytes_recvd);
    struct io_uring_sqe *sqe = io_uring_get_sqe(&worker_handler->worker->ring);
    if (sqe == nullptr) {
        logger_log_error(server->logger, "Could not get a submission queue entry.");
        exit(1);
    }
    worker_handler->start_job = (struct uring_job) {
        .routine = on_new_tftp_request,
        .args = worker_handler,
    };
    io_uring_prep_nop(sqe);
    io_uring_sqe_set_data(sqe, &worker_handler->start_job);
    ret = io_uring_submit(&worker_handler->worker->ring);
    if (ret < 0) {
        logger_log_error(server->logger, "Could not submit the new TFTP request. %s", strerror(-ret));
        exit(1);
    }
    submit_on_data_available_request(server);
}

static void on_data_available_request_removed(int32_t ret, void *args) {
    struct tftp_server *server = args;
    server->is_on_data_available_job_pending = false;
}

static void on_new_tftp_request(int32_t ret, void *args) {
    struct uring_worker_tftp_handler *worker_handler = args;
    struct tftp_server *server = worker_handler->new_request_args.server;
    uint16_t code;
    // We use memcpy since loading after casting to (uint16_t *) without 2 byte alignment is UB
    memcpy(&code, worker_handler->new_request_args.buffer, sizeof code);
    code = ntohs(code);
    if (code != TFTP_OPCODE_RRQ) {
        logger_log_warn(server->logger, "Unexpected TFTP opcode %d, expected %d.", code, TFTP_OPCODE_RRQ);
        uring_thread_pool_after_handler_closed(&worker_handler->handler);
        return;
    }
    size_t token_length = 0;
    size_t option_size = worker_handler->new_request_args.bytes_recvd - 2;
    const char *option_ptr = (const char *) &worker_handler->new_request_args.buffer[2];
    const char *end_ptr = (const char *) (&option_ptr[option_size - 1]);
    while (option_ptr != nullptr && option_ptr < end_ptr) {
        option_ptr = memchr(option_ptr, '\0', end_ptr - option_ptr + 1);
        if (option_ptr != nullptr) {
            token_length++;
            option_ptr++;
        }
    }
    option_ptr = (const char *) &worker_handler->new_request_args.buffer[2];
    if (token_length < 2 || token_length % 2 != 0) {
        logger_log_warn(server->logger, "Received ill formed packet, ignoring (tokens length: %zu).", token_length);
        uring_thread_pool_after_handler_closed(&worker_handler->handler);
        return;
    }
    tftp_handler_init(&worker_handler->handler,
                      &worker_handler->worker->ring,
                      &server->addrinfo,
                      worker_handler->new_request_args.peer_addr,
                      worker_handler->new_request_args.peer_addrlen,
                      worker_handler->new_request_args.is_orig_dest_addr_ipv4,
                      server->root,
                      option_size,
                      option_ptr,
                      server->retries,
                      server->timeout,
                      server->handler_stats_callback,
                      server->logger,
                      uring_thread_pool_after_handler_closed);
    mtx_lock(&server->stats.mtx);
    server->stats.counters.sessions_count++;
    mtx_unlock(&server->stats.mtx);
    tftp_handler_start(&worker_handler->handler);
}
