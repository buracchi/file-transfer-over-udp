#include <buracchi/cutest/cutest.h>

#include <buracchi/tftp/client.h>

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <logger.h>

#include "mock_logger.h"

TEST(client, on_unexpected_peer_do_not_terminate) {
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(1234),
    };
    int expected_socket;
    ASSERT_NE(expected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(bind(expected_socket, (struct sockaddr *)&expected_addr, sizeof(expected_addr)), -1);
    
    struct sockaddr_in6 unexpected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(2345),
    };
    int unexpected_socket;
    ASSERT_NE(unexpected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(bind(unexpected_socket, (struct sockaddr *)&unexpected_addr, sizeof(unexpected_addr)), -1);
    
    if (fork() == 0) {
        auto ret = tftp_client_get(&(struct tftp_client) {.logger = &(struct logger) {}},
                                   &(struct tftp_client_server_address) {
                                       .host = "::",
                                       .port = "1234",
                                   },
                                   &(struct tftp_client_get_request) {
                                       .filename = "garbage",
                                       .mode = TFTP_MODE_OCTET,
                                   },
                                   nullptr);
        exit(ret.success ? EXIT_SUCCESS : EXIT_FAILURE);
    }
    
    char buffer[516];
    struct sockaddr_in6 client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t received = recvfrom(expected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 1);
    sendto(expected_socket, buffer, sizeof buffer, 0, (struct sockaddr *)&client_addr, addr_len);
    received = recvfrom(expected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    sendto(unexpected_socket, buffer, sizeof buffer, 0, (struct sockaddr *)&client_addr, addr_len);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 2);
    sendto(expected_socket, buffer, sizeof buffer - 1, 0, (struct sockaddr *)&client_addr, addr_len); // send of less than block size bytes end the transfer
    received = recvfrom(expected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    int status;
    wait(&status);
    ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
}

TEST(client, on_unexpected_peer_send_error_packet) {
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(1234),
    };
    int expected_socket;
    ASSERT_NE(expected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(expected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(bind(expected_socket, (struct sockaddr *)&expected_addr, sizeof(expected_addr)), -1);
    
    struct sockaddr_in6 unexpected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(2345),
    };
    int unexpected_socket;
    ASSERT_NE(unexpected_socket = socket(AF_INET6, SOCK_DGRAM, 0), -1);
    ASSERT_NE(setsockopt(unexpected_socket, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int)), -1);
    ASSERT_NE(bind(unexpected_socket, (struct sockaddr *)&unexpected_addr, sizeof(unexpected_addr)), -1);
    
    if (fork() == 0) {
        tftp_client_get(&(struct tftp_client) {.logger = &(struct logger) {}},
                        &(struct tftp_client_server_address) {
                            .host = "::",
                            .port = "1234",
                        },
                        &(struct tftp_client_get_request) {
                            .filename = "garbage",
                            .mode = TFTP_MODE_OCTET,
                        },
                        nullptr);
        exit(0);
    }
    
    char buffer[516];
    struct sockaddr_in6 client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t received = recvfrom(expected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    tftp_data_packet_init((struct tftp_data_packet *)buffer, 1);
    sendto(expected_socket, buffer, sizeof buffer, 0, (struct sockaddr *)&client_addr, addr_len);
    received = recvfrom(expected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    sendto(unexpected_socket, buffer, sizeof buffer, 0, (struct sockaddr *)&client_addr, addr_len);
    received = recvfrom(unexpected_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    ASSERT_GT(received, 0);
    struct tftp_error_packet *error_packet = (struct tftp_error_packet *)buffer;
    ASSERT_EQ(error_packet->opcode, htons(TFTP_OPCODE_ERROR));
    ASSERT_EQ(error_packet->error_code, htons(TFTP_ERROR_UNKNOWN_TRANSFER_ID));
    wait(nullptr);
}
