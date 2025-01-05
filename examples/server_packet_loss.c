#include "server_packet_loss.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <threads.h>

#include <logger.h>
#include <string.h>

#include "dispatcher.h"

enum recv_type {
    RECVMSG,
    RECVMSG_WITH_TIMEOUT
};

union recv_params {
    struct {
        struct dispatcher *dispatcher;
        struct dispatcher_event *event;
        int fd;
        struct msghdr *msghdr;
        unsigned flags;
    } recvmsg;
    struct {
        struct dispatcher *dispatcher;
        struct dispatcher_event *recvmsg_event;
        struct dispatcher_event_timeout *timeout_event;
        int fd;
        struct msghdr *msghdr;
        unsigned flags;
    } recvmsg_with_timeout;
};

struct recv_data {
    enum recv_type type;
    union recv_params params;
    struct dispatcher_event_timeout discard_event;
    bool to_free;
};

static double packet_loss_probability = 0;
static struct logger *global_logger = nullptr;
static struct dispatcher global_dispatcher;
static int devNull = -1;

_Noreturn static int packet_discard_thread(void *);

void packet_loss_init(struct logger logger[static 1], double probability, int n) {
    thrd_t thread;
    srand(0);
    global_logger = logger;
    packet_loss_probability = probability;
    if (!dispatcher_init(&global_dispatcher, n, logger)) {
        exit(EXIT_FAILURE);
    }
    devNull = open("/dev/null", O_WRONLY);
    if (devNull == -1) {
        logger_log_fatal(global_logger, "Can not open '/dev/null' as the sink for out-coming simulated lost packet.\n");
        exit(EXIT_FAILURE);
    }
    if (thrd_create(&thread, packet_discard_thread, nullptr) != thrd_success) {
        logger_log_fatal(global_logger, "Could not create the packet discard thread.\n");
        exit(EXIT_FAILURE);
    }
    if (thrd_detach(thread) != thrd_success) {
        logger_log_fatal(global_logger, "Could not detach the packet discard thread.\n");
        exit(EXIT_FAILURE);
    }
}

// NOLINTBEGIN(*-reserved-identifier)

bool __real_dispatcher_submit_recvmsg(struct dispatcher dispatcher[static 1],
                                      struct dispatcher_event event[static 1],
                                      int fd,
                                      struct msghdr msghdr[static 1],
                                      unsigned flags);

bool __wrap_dispatcher_submit_recvmsg(struct dispatcher dispatcher[static 1],
                                      struct dispatcher_event event[static 1],
                                      int fd,
                                      struct msghdr msghdr[static 1],
                                      unsigned flags) {
    if ((rand() / (double) RAND_MAX) < packet_loss_probability) {
        struct recv_data *data = malloc(sizeof *data);
        *data = (struct recv_data) {
            .type = RECVMSG,
            .params = {
                .recvmsg = {
                    .dispatcher = dispatcher,
                    .event = event,
                    .fd = fd,
                    .msghdr = msghdr,
                    .flags = flags
                }
            },
            .discard_event = {
                .event = {
                    .id = (uint64_t) data
                }
            }
        };
        return __real_dispatcher_submit_recvmsg(
            &global_dispatcher,
            &data->discard_event.event,
            fd,
            msghdr,
            flags);
    }
    return __real_dispatcher_submit_recvmsg(dispatcher, event, fd, msghdr, flags);
}


bool __real_dispatcher_submit_recvmsg_with_timeout(struct dispatcher dispatcher[static 1],
                                                   struct dispatcher_event recvmsg_event[static 1],
                                                   struct dispatcher_event_timeout timeout_event[static 1],
                                                   int fd,
                                                   struct msghdr msghdr[static 1],
                                                   unsigned flags);

bool __wrap_dispatcher_submit_recvmsg_with_timeout(struct dispatcher dispatcher[static 1],
                                                   struct dispatcher_event recvmsg_event[static 1],
                                                   struct dispatcher_event_timeout timeout_event[static 1],
                                                   int fd,
                                                   struct msghdr msghdr[static 1],
                                                   unsigned flags) {
    if ((rand() / (double) RAND_MAX) < packet_loss_probability) {
        struct recv_data *data = malloc(sizeof *data);
        *data = (struct recv_data) {
            .type = RECVMSG_WITH_TIMEOUT,
            .params = {
                .recvmsg_with_timeout = {
                    .dispatcher = dispatcher,
                    .recvmsg_event = recvmsg_event,
                    .timeout_event = timeout_event,
                    .fd = fd,
                    .msghdr = msghdr,
                    .flags = flags
                }
            },
            .discard_event = {
                .event = {
                    .id = (uint64_t) data
                },
                .timeout = timeout_event->timeout,
            }
        };
        return __real_dispatcher_submit_recvmsg_with_timeout(
            &global_dispatcher,
            &data->discard_event.event,
            &data->discard_event,
            fd,
            msghdr,
            flags);
    }
    return __real_dispatcher_submit_recvmsg_with_timeout(dispatcher, recvmsg_event, timeout_event, fd, msghdr, flags);
}


bool __real_dispatcher_submit_sendto(struct dispatcher dispatcher[static 1],
                                     struct dispatcher_event event[static 1],
                                     int fd,
                                     const void *buf, size_t len, int flags,
                                     const struct sockaddr *addr,
                                     socklen_t addrlen);

bool __wrap_dispatcher_submit_sendto(struct dispatcher dispatcher[static 1],
                                     struct dispatcher_event event[static 1],
                                     int fd,
                                     const void *buf, size_t len, int flags,
                                     const struct sockaddr *addr,
                                     socklen_t addrlen) {
    if ((rand() / (double) RAND_MAX) < packet_loss_probability) {
        logger_log_debug(global_logger, "Next packet to be sent will be discarded to simulate packet loss.");
        struct io_uring_sqe *sqe = io_uring_get_sqe(&dispatcher->ring);
        if (sqe == nullptr) {
            logger_log_error(dispatcher->logger, "Could not get a submission queue entry.");
            return false;
        }
        io_uring_prep_write(sqe, devNull, buf, len, 0);
        io_uring_sqe_set_data(sqe, event);
        const int ret = io_uring_submit(&dispatcher->ring);
        if (ret < 0) {
            logger_log_error(dispatcher->logger, "Could not submit write request: %s", strerror(-ret));
            return false;
        }
        dispatcher->pending_requests++;
        return true;
    }
    return __real_dispatcher_submit_sendto(dispatcher, event, fd, buf, len, flags, addr, addrlen);
}

// NOLINTEND(*-reserved-identifier)

_Noreturn static int packet_discard_thread(void *) {
    struct dispatcher_event *event;
    while (true) {
        dispatcher_wait_event(&global_dispatcher, &event);
        logger_log_debug(global_logger, "Received packet was discarded to simulate packet loss.");
        if (event == nullptr) {
            if (errno == EINTR) {
                continue;
            }
            logger_log_fatal(global_logger, "Packet loss component encountered a fatal error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        auto const data = (struct recv_data *) (event->id);
        switch (data->type) {
            case RECVMSG:
                __real_dispatcher_submit_recvmsg(
                    data->params.recvmsg.dispatcher,
                    data->params.recvmsg.event,
                    data->params.recvmsg.fd,
                    data->params.recvmsg.msghdr,
                    data->params.recvmsg.flags);
                free(data);
                break;
            case RECVMSG_WITH_TIMEOUT:
                if (data->to_free) {
                    free(data);
                    break;
                }
                __real_dispatcher_submit_recvmsg_with_timeout(
                    data->params.recvmsg_with_timeout.dispatcher,
                    data->params.recvmsg_with_timeout.recvmsg_event,
                    data->params.recvmsg_with_timeout.timeout_event,
                    data->params.recvmsg_with_timeout.fd,
                    data->params.recvmsg_with_timeout.msghdr,
                    data->params.recvmsg_with_timeout.flags);
                data->to_free = true;
                break;
        }
    }
}
