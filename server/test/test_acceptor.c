#include <buracchi/cutest/cutest.h>
#include <buracchi/common/logger/logger.h>

#include "acceptor.h"

#define STR_(s) #s
#define STR(s) STR_(s)
#define DEFAULT_PORT 1234
#define DATA_PKT_SIZE 512

TEST(acceptor, bound_to_host) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    int result = 0;
    struct acceptor acceptor = {};
    result |= acceptor_init(&acceptor, STR(DEFAULT_PORT));
    result |= acceptor_destroy(&acceptor);
    ASSERT_EQ(result, 0);
}

TEST(acceptor, receive_packet) {
    cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_OFF);
    struct acceptor acceptor = {};
    if (acceptor_init(&acceptor, STR(DEFAULT_PORT))) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Acceptor initialization failed.");
        return;
    }

    // Send a message to the acceptor
    const char* message_sent = "Hello, acceptor!";
    evutil_socket_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == EVUTIL_INVALID_SOCKET) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Socket initialization failed..");
        return;
    }
    struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_addr = {
                    .s_addr = htonl(INADDR_LOOPBACK),
            },
            .sin_port = htons((DEFAULT_PORT)),
    };
    if (sendto(s, message_sent, strlen(message_sent), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Failed to send message.");
        return;
    }

    // receive message
    char message_recv[DATA_PKT_SIZE + 1] = {0};
    evutil_socket_t client_socket = EVUTIL_INVALID_SOCKET;
    struct acceptor_packet packet = {};
    if (!acceptor_accept(&acceptor, &client_socket, &packet)) {
        cutest_test_fail_(__func__, __FILE__, __LINE__, "Failed to accept message.");
        return;
    }

    acceptor_destroy(&acceptor);
    memcpy(message_recv, packet.data, packet.payload_size);
    ASSERT_STREQ(message_sent, message_recv);
}
