#include <buracchi/cutest/cutest.h>
#include <buracchi/common/logger/logger.h>

#include "acceptor.h"

#define STR_(s) #s
#define STR(s) STR_(s)
#define DEFAULT_PORT 1234
#define DATA_PKT_SIZE 512

static const char *send_to_default_acceptor(int protocol_family, size_t n, const char message[n]);

TEST(acceptor, can_bound_to_host) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    int result = 0;
    struct acceptor acceptor = {};
    result |= acceptor_init(&acceptor, STR(DEFAULT_PORT));
    result |= acceptor_destroy(&acceptor);
    ASSERT_EQ(result, 0);
}

TEST(acceptor, receive_test_packet) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    struct acceptor acceptor = {};
    if (acceptor_init(&acceptor, STR(DEFAULT_PORT))) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Acceptor initialization failed.");
        return;
    }
    const char message_sent[] = "Hello, acceptor!";
    const char *err = send_to_default_acceptor(AF_UNSPEC, sizeof message_sent, message_sent);
    if (err != nullptr) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, err);
        return;
    }
    char message_recv[DATA_PKT_SIZE + 1] = {0};
    evutil_socket_t accpted_socket = EVUTIL_INVALID_SOCKET;
    struct acceptor_packet packet = {};
    accpted_socket = acceptor_accept(&acceptor, &packet);
    if (accpted_socket == EVUTIL_INVALID_SOCKET) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Failed to accept message.");
        return;
    }
    memcpy(message_recv, packet.data, packet.payload_size);
    ASSERT_STREQ(message_sent, message_recv);
    evutil_closesocket(accpted_socket);
    acceptor_destroy(&acceptor);
}

TEST(acceptor, support_ipv4_requests) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    struct acceptor acceptor = {};
    if (acceptor_init(&acceptor, STR(DEFAULT_PORT))) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Acceptor initialization failed.");
        return;
    }
    const char *err = send_to_default_acceptor(AF_INET, 1, &(char) {0x00});
    if (err != nullptr) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, err);
        return;
    }
    evutil_socket_t accpted_socket = EVUTIL_INVALID_SOCKET;
    struct acceptor_packet packet = {};
    accpted_socket = acceptor_accept(&acceptor, &packet);
    ASSERT_NE(accpted_socket, EVUTIL_INVALID_SOCKET);
    evutil_closesocket(accpted_socket);
    acceptor_destroy(&acceptor);
}

TEST(acceptor, support_ipv6_requests) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    struct acceptor acceptor = {};
    if (acceptor_init(&acceptor, STR(DEFAULT_PORT))) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Acceptor initialization failed.");
        return;
    }
    const char *err = send_to_default_acceptor(AF_INET6, 1, &(char) {0x00});
    if (err != nullptr) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, err);
        return;
    }
    evutil_socket_t accpted_socket = EVUTIL_INVALID_SOCKET;
    struct acceptor_packet packet = {};
    accpted_socket = acceptor_accept(&acceptor, &packet);
    ASSERT_NE(accpted_socket, EVUTIL_INVALID_SOCKET);
    evutil_closesocket(accpted_socket);
    acceptor_destroy(&acceptor);
}

static const char *send_to_default_acceptor(int protocol_family, size_t n, const char message[n]) {
    struct addrinfo *clnt_ainfo;
    struct evutil_addrinfo hints = {
        .ai_family = protocol_family,
        .ai_socktype = SOCK_DGRAM,
    };
    int gai_ret = evutil_getaddrinfo(nullptr, STR(DEFAULT_PORT), &hints, &clnt_ainfo);
    if (gai_ret != 0) {
        return "Failed to get address info.";
    }
    evutil_socket_t client_socket = socket(clnt_ainfo->ai_family, clnt_ainfo->ai_socktype, clnt_ainfo->ai_protocol);
    if (client_socket == EVUTIL_INVALID_SOCKET) {
        return "Socket initialization failed.";
    }
    if (connect(client_socket, clnt_ainfo->ai_addr, clnt_ainfo->ai_addrlen) == -1) {
        return "Failed to connect to the server.";
    }
    evutil_freeaddrinfo(clnt_ainfo);
    if (send(client_socket, message, n, 0) == -1) {
        return "Failed to send message.";
    }
    return nullptr;
}
