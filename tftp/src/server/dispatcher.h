#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdint.h>

#include <liburing.h>
#include <logger.h>

struct dispatcher {
    struct io_uring ring;
    struct logger *logger;
    uint8_t pending_requests;
};

struct dispatcher_event {
    uint64_t id;
    bool is_success;
    union {
        int32_t result;
        int32_t error_number;
    };
};

struct dispatcher_event_timeout {
    struct dispatcher_event event;
    struct __kernel_timespec timeout;
};

bool dispatcher_init(struct dispatcher dispatcher[static 1], uint32_t max_requests, struct logger logger[static 1]);

bool dispatcher_destroy(struct dispatcher dispatcher[static 1]);

bool dispatcher_wait_event(struct dispatcher dispatcher[static 1], struct dispatcher_event *event[static 1]);

bool dispatcher_submit(struct dispatcher dispatcher[static 1], struct dispatcher_event *event);

bool dispatcher_submit_timeout(struct dispatcher dispatcher[static 1],
                               struct dispatcher_event_timeout event[static 1],
                               int seconds);

bool dispatcher_submit_readv(struct dispatcher dispatcher[static 1],
                             struct dispatcher_event event[static 1],
                             int fd,
                             const struct iovec *iovecs,
                             unsigned nr_vecs);

bool dispatcher_submit_recvmsg(struct dispatcher dispatcher[static 1],
                               struct dispatcher_event event[static 1],
                               int fd,
                               struct msghdr msghdr[static 1],
                               unsigned flags);

bool dispatcher_submit_recvmsg_with_timeout(struct dispatcher dispatcher[static 1],
                                            struct dispatcher_event recvmsg_event[static 1],
                                            struct dispatcher_event_timeout timeout_event[static 1],
                                            int fd,
                                            struct msghdr msghdr[static 1],
                                            unsigned flags);

bool dispatcher_submit_sendto(struct dispatcher dispatcher[static 1],
                              struct dispatcher_event event[static 1],
                              int fd,
                              const void *buf, size_t len, int flags,
                              const struct sockaddr *addr,
                              socklen_t addrlen);

bool dispatcher_submit_cancel(struct dispatcher dispatcher[static 1],
                              struct dispatcher_event *event,
                              struct dispatcher_event event_to_cancel[static 1]);

bool dispatcher_submit_cancel_timeout(struct dispatcher dispatcher[static 1],
                                      struct dispatcher_event *event,
                                      struct dispatcher_event_timeout event_to_cancel[static 1]);

#endif // DISPATCHER_H
