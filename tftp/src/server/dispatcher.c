#include "dispatcher.h"

#include <string.h>

bool dispatcher_init(struct dispatcher dispatcher[static 1], uint32_t max_requests, struct logger logger[static 1]) {
    *dispatcher = (struct dispatcher) {
            .logger = logger,
            .pending_requests = 0,
    };
    dispatcher->logger = logger;
    int ret = io_uring_queue_init(max_requests, &dispatcher->ring, 0);
    if (ret < 0) {
        logger_log_error(logger, "Failed to initialize server io_uring: %s.", strerror(-ret));
        return false;
    }
    return true;
}

bool dispatcher_destroy(struct dispatcher dispatcher[static 1]) {
    io_uring_queue_exit(&dispatcher->ring);
    return true;
}

bool dispatcher_wait_event(struct dispatcher dispatcher[static 1], struct dispatcher_event *event[static 1]) {
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&dispatcher->ring, &cqe);
    if (ret < 0) {
        errno = -ret;
        return false;
    }
    *event = io_uring_cqe_get_data(cqe);
    if (*event != nullptr) {
        if (cqe->res < 0) {
            (*event)->is_success = false;
            (*event)->error_number = -cqe->res;
        }
        else {
            (*event)->is_success = true;
            (*event)->result = cqe->res;
        }
    }
    io_uring_cqe_seen(&dispatcher->ring, cqe);
    dispatcher->pending_requests--;
    return true;
}

bool dispatcher_submit(struct dispatcher dispatcher[static 1], struct dispatcher_event *event) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a sqe from the ring.");
        return false;
    }
    io_uring_prep_nop(sqe);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not submit a NOP request to the ring. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_timeout(struct dispatcher dispatcher[static 1], struct dispatcher_event_timeout event[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_timeout(sqe, &event->timeout, 0, 0);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not submit the timeout request. %s", strerror(-ret));
        return false;
    }
    logger_log_trace(dispatcher->logger, "Timeout set to %lld.%.9lds.", event->timeout.tv_sec, event->timeout.tv_nsec);
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_timeout_update(struct dispatcher dispatcher[static 1],
                                      struct dispatcher_event event[static 1],
                                      struct dispatcher_event_timeout event_to_update[static 1],
                                      struct __kernel_timespec timeout[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_timeout_update(sqe, timeout, (size_t) event_to_update, 0);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not update the timeout request. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_timeout_cancel(struct dispatcher dispatcher[static 1],
                                      struct dispatcher_event event[static 1],
                                      struct dispatcher_event_timeout event_to_cancel[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_timeout_remove(sqe, (size_t) event_to_cancel, 0);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not remove the timeout request. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_read(struct dispatcher dispatcher[static 1], struct dispatcher_event event[static 1], int fd, void *buffer, unsigned n_bytes) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_read(sqe, fd, buffer, n_bytes, -1);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not submit read request. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_recvmsg(struct dispatcher dispatcher[static 1], struct dispatcher_event event[static 1], int fd, struct msghdr msghdr[static 1], unsigned flags) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_recvmsg(sqe, fd, msghdr, flags);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not submit recvmsg request. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_sendto(struct dispatcher dispatcher[static 1],
                              struct dispatcher_event event[static 1],
                              int fd,
                              const void *buf,
                              size_t len,
                              int flags,
                              const struct sockaddr *addr,
                              socklen_t addrlen) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_sendto(sqe, fd, buf, len, flags, addr, addrlen);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not submit sendto request: %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}

bool dispatcher_submit_cancel(struct dispatcher dispatcher[static 1], struct dispatcher_event *event, struct dispatcher_event event_to_cancel[static 1]) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
    if (sqe == nullptr) {
        logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
        return false;
    }
    io_uring_prep_cancel(sqe, event_to_cancel, 0);
    io_uring_sqe_set_data(sqe, event);
    int ret = io_uring_submit(&dispatcher->ring);
    if (ret < 0) {
        logger_log_error(dispatcher->logger, "Could not remove the on data available request. %s", strerror(-ret));
        return false;
    }
    dispatcher->pending_requests++;
    return true;
}
