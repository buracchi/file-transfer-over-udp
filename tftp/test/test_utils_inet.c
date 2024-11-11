#include <buracchi/cutest/cutest.h>

#include <arpa/inet.h>

#include "../src/utils/inet.h"

TEST(InetUtils, InetNtopAddress) {
    // Test for IPv4 address
    char const *result;
    struct sockaddr_in ipv4_address = {
        .sin_family = AF_INET,
        .sin_port = htons(1234),
    };
    char address_str[INET6_ADDRSTRLEN];
    uint16_t port;
    inet_pton(AF_INET, "127.0.0.1", &ipv4_address.sin_addr);
    result = inet_ntop_address((struct sockaddr *)&ipv4_address, address_str, &port);
    ASSERT_STREQ(result, "127.0.0.1");
    ASSERT_EQ(port, 1234);

    // Test for IPv6 address
    struct sockaddr_in6 ipv6_address = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(5678),
    };
    inet_pton(AF_INET6, "::1", &ipv6_address.sin6_addr);
    result = inet_ntop_address((struct sockaddr *)&ipv6_address, address_str, &port);
    ASSERT_STREQ(result, "::1");
    ASSERT_EQ(port, 5678);
}

TEST(InetUtils, InetAddrInToIn6) {
    // Test conversion of IPv4 address
    struct sockaddr_in ipv4_address = {
        .sin_family = AF_INET,
        .sin_port = htons(1234),
    };
    inet_pton(AF_INET, "127.0.0.1", &ipv4_address.sin_addr);

    struct sockaddr_in6 ipv6_address = {};
    char expected_addr_str[INET6_ADDRSTRLEN];
    int result = inet_addr_in_to_in6(&ipv4_address, &ipv6_address);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ipv6_address.sin6_family, AF_INET6);
    ASSERT_EQ(ipv6_address.sin6_port, ipv4_address.sin_port);
    inet_ntop(AF_INET6, &ipv6_address.sin6_addr, expected_addr_str, INET6_ADDRSTRLEN);
    ASSERT_STREQ(expected_addr_str, "::ffff:127.0.0.1");
}
