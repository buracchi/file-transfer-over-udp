#include <buracchi/cutest/cutest.h>

#include <buracchi/tftp/client.h>

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <logger.h>

#include "mock_logger.h"

constexpr struct timeval recv_timeout = { .tv_sec = 2, .tv_usec = 0 };

TEST(client, on_unexpected_peer_do_not_terminate) {
    uint16_t server_port = 1234;
    char server_port_str[6];
    snprintf(server_port_str, sizeof server_port_str, "%hu", server_port);
    FILE *out = fopen("/dev/null", "w");
    ASSERT_NE(out, nullptr);
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(server_port),
    };
    int expected_socket;
    ASSERT_NE(expected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)), -1);
    ASSERT_NE(bind(expected_socket, (struct sockaddr *)&expected_addr, sizeof(expected_addr)), -1);
    
    struct sockaddr_in6 unexpected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(2345),
    };
    int unexpected_socket;
    ASSERT_NE(unexpected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)), -1);
    ASSERT_NE(bind(unexpected_socket, (struct sockaddr *)&unexpected_addr, sizeof(unexpected_addr)), -1);

    if (fork() == 0) {
        struct logger logger;
        logger_init(&logger, logger_default_config);
        logger.config.default_level = LOGGER_LOG_LEVEL_ALL;
        auto const ret = tftp_client_read(&logger, 0, "::", server_port_str, "test", TFTP_MODE_OCTET, nullptr, out);
        exit(ret.is_success ? EXIT_SUCCESS : EXIT_FAILURE);
    }
    
    char buffer[516];
    struct sockaddr_in6 client_addr_in6;
    socklen_t addr_len = sizeof(client_addr_in6);
    struct sockaddr *client_addr = (struct sockaddr *) &client_addr_in6;
    ssize_t ret;
    
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, client_addr, &addr_len);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 1);
    sendto(expected_socket, buffer, sizeof buffer, 0, client_addr, addr_len);
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    sendto(unexpected_socket, buffer, sizeof buffer, 0, client_addr, addr_len);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 2);
    sendto(expected_socket, buffer, sizeof buffer - 1, 0, client_addr, addr_len); // send of less than block size bytes end the transfer
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    close(unexpected_socket);
    close(expected_socket);
    int status;
    wait(&status);
    ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
}

TEST(client, on_unexpected_peer_send_error_packet) {
    uint16_t server_port = 1235;
    char server_port_str[6];
    snprintf(server_port_str, sizeof server_port_str, "%hu", server_port);
    FILE *out = fopen("/dev/null", "w");
    ASSERT_NE(out, nullptr);
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(server_port),
    };
    int expected_socket;
    ASSERT_NE(expected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)), -1);
    ASSERT_NE(bind(expected_socket, (struct sockaddr *)&expected_addr, sizeof(expected_addr)), -1);
    
    struct sockaddr_in6 unexpected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(3456),
    };
    int unexpected_socket;
    ASSERT_NE(unexpected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)), -1);
    ASSERT_NE(bind(unexpected_socket, (struct sockaddr *)&unexpected_addr, sizeof(unexpected_addr)), -1);
    
    if (fork() == 0) {
        tftp_client_read(&(struct logger) {}, 0, "::", server_port_str, "test", TFTP_MODE_OCTET, nullptr, out);
        exit(0);
    }
    
    char buffer[516];
    struct sockaddr_in6 client_addr_in6;
    socklen_t addr_len = sizeof(client_addr_in6);
    struct sockaddr *client_addr = (struct sockaddr *) &client_addr_in6;
    ssize_t ret;
    
    ret = recvfrom(expected_socket, buffer, sizeof(buffer), 0, client_addr, &addr_len);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 1);
    sendto(expected_socket, buffer, sizeof buffer, 0, client_addr, addr_len);
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    sendto(unexpected_socket, buffer, sizeof buffer, 0, client_addr, addr_len);
    ret = recvfrom(unexpected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    ASSERT_GT(ret, 0);
    auto const error_packet = (struct tftp_error_packet *)buffer;
    ASSERT_EQ(error_packet->opcode, htons(TFTP_OPCODE_ERROR));
    ASSERT_EQ(error_packet->error_code, htons(TFTP_ERROR_UNKNOWN_TRANSFER_ID));
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 2);
    sendto(expected_socket, buffer, sizeof buffer - 1, 0, client_addr, addr_len); // send of less than block size bytes end the transfer
    close(unexpected_socket);
    close(expected_socket);
    wait(nullptr);
}
