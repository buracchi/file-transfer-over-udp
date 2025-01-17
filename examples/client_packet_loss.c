#include "client_packet_loss.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include <logger.h>

static double packet_loss_probability = 0;
static struct logger *global_logger = nullptr;

void packet_loss_init(struct logger logger[static 1], double probability, bool disable_fixed_seed) {
    if (!disable_fixed_seed) {
        srand(0);
    } else {
        srand((unsigned)time(nullptr));
    }
    global_logger = logger;
    packet_loss_probability = probability;
}

// NOLINTBEGIN(*-reserved-identifier)

ssize_t __real_recvfrom(int sockfd, void *buffer, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t __wrap_recvfrom(int sockfd, void *buffer, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
    const ssize_t ret = __real_recvfrom(sockfd, buffer, len, flags, src_addr, addrlen);
    if (ret < 0) {
        return ret;
    }
    if ((rand() / (double) RAND_MAX) < packet_loss_probability) {
        logger_log_debug(global_logger, "Received packet was discarded to simulate packet loss.");
        return __wrap_recvfrom(sockfd, buffer, len, flags, src_addr, addrlen);
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
