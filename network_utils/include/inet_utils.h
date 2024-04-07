#pragma once

#include <netinet/in.h>

/**
 * Converts the provided socket address to a string representation.
 *
 * @param address a pointer to the socket address structure to convert
 * @param address_str a buffer to store the resulting string representation
 * @param port a pointer to a uint16_t to store the port number of the address (optional)
 * @return a pointer to the resulting string representation on success, NULL on failure
 */
const char *inet_ntop_address(const struct sockaddr address[static 1], char address_str[static INET6_ADDRSTRLEN], uint16_t port[static 1]);

/**
 * Converts a sockaddr_in structure to a sockaddr_in6 structure.
 * The src parameter sin_family field must be set to AF_INET before calling this function.
 * The sockaddr_in protocol must be
 *
 * @param src a pointer to the sockaddr_in structure to convert
 * @param dest a pointer to the sockaddr_in6 structure to store the result
 * @return 0 on success, -1 on failure
 */
int inet_addr_in_to_in6(struct sockaddr_in const src[restrict static 1], struct sockaddr_in6 dest[restrict static 1]);

int inet_add_in6_to_in(struct sockaddr_in6 const src[restrict static 1], struct sockaddr_in dest[restrict static 1]);

/**
 * Prints detailed information about the provided socket descriptor.
 *
 * @param fd the socket descriptor
 */
void print_inet_address_detail(int fd);
