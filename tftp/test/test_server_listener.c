#include <buracchi/cutest/cutest.h>

#include <buracchi/tftp/server_listener.h>

#include <unistd.h>

#include <logger.h>

#include "mock_logger.h"

constexpr const char port[] = "6969";
constexpr const char invalid_port[] = "test";
constexpr const char ipv6_address[] = "::";
constexpr const char ipv4_address[] = "127.0.0.1";
constexpr const char localhost_address[] = "localhost";
constexpr const char invalid_address[] = "invalid_address";

TEST(server_listener, initialize_for_localhost) {
    struct tftp_server_listener listener;
    struct logger logger;
    ASSERT_TRUE(tftp_server_listener_init(&listener, localhost_address, port, &logger));
    ASSERT_NE(listener.file_descriptor, -1);
    ASSERT_NE(listener.msghdr.msg_control, NULL);
    tftp_server_listener_destroy(&listener);
}

TEST(server_listener, initialize_for_ipv4_address) {
    struct tftp_server_listener listener;
    struct logger logger;
    ASSERT_TRUE(tftp_server_listener_init(&listener, ipv4_address, port, &logger));
    ASSERT_NE(listener.file_descriptor, -1);
    ASSERT_NE(listener.msghdr.msg_control, NULL);
    tftp_server_listener_destroy(&listener);
}

TEST(server_listener, initialize_for_ipv6_address) {
    struct tftp_server_listener listener;
    struct logger logger;
    ASSERT_TRUE(tftp_server_listener_init(&listener, ipv6_address, port, &logger));
    ASSERT_NE(listener.file_descriptor, -1);
    ASSERT_NE(listener.msghdr.msg_control, NULL);
    tftp_server_listener_destroy(&listener);
}

TEST(server_listener, do_not_initialize_for_invalid_address) {
    struct tftp_server_listener listener;
    struct logger logger;
    ASSERT_FALSE(tftp_server_listener_init(&listener, invalid_address, port, &logger));
    tftp_server_listener_destroy(&listener);
    ASSERT_FALSE(tftp_server_listener_init(&listener, localhost_address, invalid_port, &logger));
    tftp_server_listener_destroy(&listener);
}

TEST(server_listener, recvmsg_use_message_setted) {
    struct tftp_server_listener listener;
    struct tftp_peer_message message;
    struct logger logger;
    tftp_server_listener_init(&listener, localhost_address, port, &logger);
    tftp_server_listener_set_recvmsg_dest(&listener, &message);
    char payload[] = "Payload";
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    ASSERT_NE(sockfd, -1);
    ssize_t bytes_sent = sendto(sockfd, payload, sizeof payload, 0, listener.addrinfo.ai_addr, listener.addrinfo.ai_addrlen);
    ASSERT_EQ(bytes_sent, sizeof payload);
    ASSERT_EQ(close(sockfd), 0);
    ssize_t bytes_received = recvmsg(listener.file_descriptor, &listener.msghdr, 0);
    ASSERT_EQ(bytes_received, sizeof payload);
    ASSERT_STREQ((char *)message.buffer, payload);
    tftp_server_listener_destroy(&listener);
}
