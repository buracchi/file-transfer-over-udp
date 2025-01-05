#ifndef INET_UTILS_H
#define INET_UTILS_H

#include <netinet/in.h>

/**
 * Converts the provided socket address to a string representation.
 *
 * @param address a pointer to the socket address structure to convert
 * @param address_str a buffer to store the resulting string representation
 * @param port a pointer to a uint16_t to store the port number of the address (optional)
 * @return a pointer to the resulting string representation on success, NULL on failure
 */
const char *sockaddr_ntop(const struct sockaddr address[static 1], char address_str[static INET6_ADDRSTRLEN], uint16_t port[static 1]);

/**
 * Converts a sockaddr_in structure to a sockaddr_in6 structure.
 * The src parameter sin_family field must be set to AF_INET before calling this function.
 * The sockaddr_in protocol must be
 *
 * @param src a pointer to the sockaddr_in structure to convert
 * @param dest a pointer to the sockaddr_in6 structure to store the result
 * @return 0 on success, -1 on failure
 */
int sockaddr_in_to_in6(const struct sockaddr_in src[static 1], struct sockaddr_in6 dest[static 1]);

int sockaddr_in6_to_in(const struct sockaddr_in6 src[static 1], struct sockaddr_in dest[static 1]);

bool sockaddr_equals(const struct sockaddr_storage lhs[static 1], const struct sockaddr_storage rhs[static 1]);

#endif // INET_UTILS_H
