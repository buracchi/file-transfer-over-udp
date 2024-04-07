#include <buracchi/cutest/cutest.h>

TEST(test, dummy) {
    ASSERT_EQ(0, 0);
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
