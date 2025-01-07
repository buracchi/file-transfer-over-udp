#include <buracchi/cutest/cutest.h>

#include <buracchi/tftp/server.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <threads.h>

#include <logger.h>

#include "mock_logger.h"

constexpr struct timeval recv_timeout = { .tv_sec = 2, .tv_usec = 0 };

int server_thread(void *arg) {
    struct tftp_server *server = (struct tftp_server *)arg;
    tftp_server_start(server);
    return 0;
}

TEST(server, on_unexpected_peer_do_not_terminate) {
    thrd_t server_thrd;
    struct tftp_server server;
    uint16_t server_port = 1234;
    char server_port_str[6];
    snprintf(server_port_str, sizeof server_port_str, "%hu", server_port);
    struct tftp_server_arguments args = {
        .ip = "::",
        .port = server_port_str,
        .root = "/dev",
        .retries = 3,
        .timeout_s = 5,
        .workers = 1,
        .max_worker_sessions = 1,
        .server_stats_callback = nullptr,
        .session_stats_callback = nullptr,
        .stats_interval_seconds = 60,
    };
    struct sockaddr_in6 server_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(server_port),
    };
    ASSERT_TRUE(tftp_server_init(&server, args, &(struct logger) {}));
    ASSERT_EQ(thrd_create(&server_thrd, server_thread, &server), thrd_success);
    
    struct tftp_rrq_packet rrq;
    char filename[] = "null";
    size_t rrq_size = tftp_rrq_packet_init(
        &rrq,
        sizeof filename,
        filename,
        TFTP_MODE_OCTET,
        (struct tftp_option[TFTP_OPTION_TOTAL_OPTIONS]) {
            [TFTP_OPTION_BLKSIZE] = {.is_active = false},
            [TFTP_OPTION_TIMEOUT] = {.is_active = false},
            [TFTP_OPTION_TSIZE] = {.is_active = false},
            [TFTP_OPTION_WINDOWSIZE] = {.is_active = false},
        });
    struct tftp_ack_packet ack;
    tftp_ack_packet_init(&ack, 1);
    
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(2345),
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
    
    struct sockaddr_storage session_addr = {};
    socklen_t session_addr_len = sizeof session_addr;
    uint8_t buffer[256] = {};
    ssize_t ret;
    
    sendto(expected_socket, &rrq, rrq_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, (struct sockaddr *) &session_addr, &session_addr_len);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    sendto(unexpected_socket, buffer, sizeof buffer, 0, (struct sockaddr *) &session_addr, session_addr_len);
    ret = recvfrom(unexpected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    sendto(expected_socket, &ack, sizeof ack, 0, (struct sockaddr *) &session_addr, session_addr_len);
    
    ASSERT_FALSE(server.should_stop);
    tftp_server_stop(&server);
    // sending invalid opcode to exit from the recvmsg server loop
    sendto(expected_socket, &(char[]){0xF}, 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    thrd_join(server_thrd, nullptr);
    close(unexpected_socket);
    close(expected_socket);
    tftp_server_destroy(&server);
}

TEST(server, on_unexpected_peer_send_error_packet) {
    thrd_t server_thrd;
    struct tftp_server server;
    uint16_t server_port = 1234;
    char server_port_str[6];
    snprintf(server_port_str, sizeof server_port_str, "%hu", server_port);
    struct tftp_server_arguments args = {
        .ip = "::",
        .port = server_port_str,
        .root = "/dev",
        .retries = 3,
        .timeout_s = 5,
        .workers = 1,
        .max_worker_sessions = 1,
        .server_stats_callback = nullptr,
        .session_stats_callback = nullptr,
        .stats_interval_seconds = 60,
    };
    struct sockaddr_in6 server_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(server_port),
    };
    ASSERT_TRUE(tftp_server_init(&server, args, &(struct logger) {}));
    ASSERT_EQ(thrd_create(&server_thrd, server_thread, &server), thrd_success);
    
    struct tftp_rrq_packet rrq;
    char filename[] = "null";
    size_t rrq_size = tftp_rrq_packet_init(
        &rrq,
        sizeof filename,
        filename,
        TFTP_MODE_OCTET,
        (struct tftp_option[TFTP_OPTION_TOTAL_OPTIONS]) {
            [TFTP_OPTION_BLKSIZE] = {.is_active = false},
            [TFTP_OPTION_TIMEOUT] = {.is_active = false},
            [TFTP_OPTION_TSIZE] = {.is_active = false},
            [TFTP_OPTION_WINDOWSIZE] = {.is_active = false},
        });
    struct tftp_ack_packet ack;
    tftp_ack_packet_init(&ack, 1);
    
    struct sockaddr_in6 expected_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_loopback,
        .sin6_port = htons(2345),
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

    struct sockaddr_storage session_addr = {};
    socklen_t session_addr_len = sizeof session_addr;
    uint8_t buffer[256] = {};
    ssize_t ret;
    
    sendto(expected_socket, &rrq, rrq_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ret = recvfrom(expected_socket, buffer, sizeof buffer, 0, (struct sockaddr *) &session_addr, &session_addr_len);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    sendto(unexpected_socket, buffer, sizeof buffer, 0, (struct sockaddr *) &session_addr, session_addr_len);
    ret = recvfrom(unexpected_socket, buffer, sizeof buffer, 0, nullptr, nullptr);
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ASSERT_FALSE("timeout");
    }
    sendto(expected_socket, &ack, sizeof ack, 0, (struct sockaddr *) &session_addr, session_addr_len);
    
    struct tftp_error_packet *error_packet = (struct tftp_error_packet *)buffer;
    ASSERT_EQ(error_packet->opcode, htons(TFTP_OPCODE_ERROR));
    ASSERT_EQ(error_packet->error_code, htons(TFTP_ERROR_UNKNOWN_TRANSFER_ID));

    tftp_server_stop(&server);
    // sending invalid opcode to exit from the recvmsg server loop
    sendto(expected_socket, &(char[]){0xF}, 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    thrd_join(server_thrd, nullptr);
    close(unexpected_socket);
    close(expected_socket);
    tftp_server_destroy(&server);
}

// testRRQ
// Send an RRQ packet with a filename, mode, and some options to the server.
// Asserts that the server correctly parses the RRQ and that the handler's attributes (addr, peer, path, and options) are set as expected.
// Example [path: ("some_file"); options: ("mode": "binascii", "opt1_key": "opt1_val", "default_timeout": 100, "retries": 200)].

// testTimer
// Verifies that the server's statistics callback is invoked after a timer is started.
// Assert that the statistics callback is triggered and executed within the wait period.

// testTimerNoCallBack
// Runs the server with no callback provided for statistics.
// Assert that no callback is executed.

// testUnexpectedOpsCode
// Send a packet with an unexpected or unknown opcode and checks how the server responds to it.
// Assert that the server does not create a handler for this invalid opcode.
// Assert that the server does not crash and correctly ignores the invalid opcode.

//TEST(acceptor, can_bound_to_host)

//TEST(acceptor, receive_test_packet)

//TEST(acceptor, support_ipv4_requests)

//TEST(acceptor, support_ipv6_requests)
