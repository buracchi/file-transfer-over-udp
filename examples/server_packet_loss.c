#include "server_packet_loss.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <threads.h>
#include <time.h>

#include <logger.h>
#include <string.h>

#include "dispatcher.h"

struct recv_data {
    struct dispatcher *dispatcher;
    struct dispatcher_event *event;
    int fd;
    struct msghdr *msghdr;
    unsigned flags;
    struct dispatcher_event discard_event;
};

static double packet_loss_probability = 0;
static struct logger *global_logger = nullptr;
static struct dispatcher global_dispatcher;
static int devNull = -1;

_Noreturn static int packet_discard_thread(void *);

void packet_loss_init(struct logger logger[static 1], double probability, int n, bool disable_fixed_seed) {
    thrd_t thread;
    if (!disable_fixed_seed) {
        srand(0);
    } else {
        srand((unsigned)time(nullptr));
    }
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
            .dispatcher = dispatcher,
            .event = event,
            .fd = fd,
            .msghdr = msghdr,
            .flags = flags,
            .discard_event = {
                .id = (uint64_t) data
            }
        };
        return __real_dispatcher_submit_recvmsg(
            &global_dispatcher,
            &data->discard_event,
            fd,
            msghdr,
            flags);
    }
    return __real_dispatcher_submit_recvmsg(dispatcher, event, fd, msghdr, flags);
}


ssize_t __real_recvmsg(int sockfd, struct msghdr *message, int flags);

ssize_t __wrap_recvmsg(int sockfd, struct msghdr *message, int flags) {
    const ssize_t ret = __real_recvmsg(sockfd, message, flags);
    if (ret < 0) {
        return ret;
    }
    if ((rand() / (double) RAND_MAX) < packet_loss_probability) {
        logger_log_debug(global_logger, "Received packet was discarded to simulate packet loss.");
        return __wrap_recvmsg(sockfd, message, flags);
    }
    return ret;
}


ssize_t __real_sendto(int sockfd, const void *buffer, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t __wrap_sendto(int sockfd, const void *buffer, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {
    if (rand() / (double) RAND_MAX < packet_loss_probability) {
        logger_log_debug(global_logger, "Packet was not sent to simulate packet loss.");
        return len;
    }
    return __real_sendto(sockfd, buffer, len, flags, dest_addr, addrlen);
}

// NOLINTEND(*-reserved-identifier)

_Noreturn static int packet_discard_thread(void *) {
    struct dispatcher_event *event;
    while (true) {
        dispatcher_wait_event(&global_dispatcher, &event);
        if (event == nullptr) {
            if (errno == EINTR) {
                continue;
            }
            logger_log_fatal(global_logger, "Packet loss component encountered a fatal error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        logger_log_debug(global_logger, "Received packet was discarded to simulate packet loss.");
        auto const data = (struct recv_data *) (event->id);
        __real_dispatcher_submit_recvmsg(
            data->dispatcher,
            data->event,
            data->fd,
            data->msghdr,
            data->flags);
        free(data);
    }
}
