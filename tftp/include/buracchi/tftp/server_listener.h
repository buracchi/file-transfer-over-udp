#ifndef TFTP_SERVER_LISTENER_H
#define TFTP_SERVER_LISTENER_H

#include <netdb.h>
#include <sys/socket.h>
#include <bits/types/struct_iovec.h>

#include <logger.h>
#include <tftp.h>

struct tftp_server_listener {
    int file_descriptor;
    struct sockaddr_storage addr_storage;
    struct addrinfo addrinfo;
    struct msghdr msghdr;
    struct iovec iovec[1];
};

struct tftp_peer_message {
    struct sockaddr_storage peer_addr;
    socklen_t peer_addrlen;
    ssize_t bytes_recvd;
    bool is_orig_dest_addr_ipv4;
    uint8_t buffer[tftp_request_packet_max_size];
};
/**
 * \brief Initializes a TFTP server listener.
 *
 * This function sets up a TFTP server listener to bind to a specified host and service (port).
 * It configures the necessary socket options and prepares the listener for receiving TFTP requests.
 *
 * \param listener A pointer to a `tftp_server_listener` structure that will be initialized.
 * \param host The hostname or IP address to bind the server to.
 * \param service A port number to bind the server to.
 * \param logger A pointer to a `logger` structure for logging errors and debug information.
 * \return `true` if the listener was successfully initialized and bound to the specified address and port, `false` otherwise.
 *
 * \note The caller is responsible for calling `tftp_server_listener_destroy` to clean up resources allocated by this function.
 */
bool tftp_server_listener_init(struct tftp_server_listener listener[static 1],
                               const char host[static 1],
                               const char service[static 1],
                               struct logger logger[static 1]);

/**
 * \brief Sets the destination address and buffer for a TFTP peer message.
 *
 * This function initializes the message header (`msghdr`) of the TFTP server listener
 * to point to the destination address and buffer of the provided TFTP peer message.
 * It also resets the control buffer and sets the message flags to zero.
 *
 * \param listener A pointer to the TFTP server listener structure.
 * \param dest A pointer to the TFTP peer message structure.
 */
void tftp_server_listener_set_recvmsg_dest(struct tftp_server_listener listener[static 1],
                                           struct tftp_peer_message dest[static 1]);

void tftp_server_listener_destroy(struct tftp_server_listener listener[static 1]);

#endif // TFTP_SERVER_LISTENER_H
