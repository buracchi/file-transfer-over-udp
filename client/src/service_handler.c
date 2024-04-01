#include "service_handler.h"

#include <stdlib.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

int service_handler_handle_request(service_handler_t* service_handler, evutil_socket_t sockfd) {
	cmn_logger_log_info("Handling the user's request.");
	if (!strcmp(service_handler->command, "list") || !strcmp(service_handler->command, "put")) {
		cmn_logger_log_error("Command not implemented yet.");
		return 1;
	}
	char buff[256] = { 0 };
	ssize_t bytes_read;
	ssize_t bytes_sent;
	uint8_t *packet;
	size_t packet_len;
	packet_len = 2 + strlen(service_handler->filename) + 1 + sizeof "octet";
	packet = malloc(packet_len);
	memcpy(packet, &(uint8_t[]){ 0x0, 0x1 }, 2);
	memcpy(packet + 2, service_handler->filename, strlen(service_handler->filename) + 1);
	memcpy(packet + 2 + strlen(service_handler->filename) + 1, "octet", sizeof "octet");
	try((bytes_sent = send(sockfd, packet, packet_len, 0)) == -1, true, send_fail);
	printf("client: sent %zu bytes\n", bytes_sent);
	printf("client: waiting reply from server...\n");
	try((bytes_read = recv(sockfd, buff, sizeof(buff), 0)) == -1, true, recv_fail);
	printf("client: received %zd bytes, packet content is \"%s\"\n", bytes_read, buff);
	printf("client: replying to server...\n");
	char _message[] = "ACK";
	bytes_sent = 0;
	while (bytes_sent != sizeof _message) {
		int n;
		try((n = send(sockfd, _message + bytes_sent, strlen(_message + bytes_sent) + 1, 0)) == -1, true, send_fail);
		bytes_sent += n;
	}
	return 0;
recv_fail:
	fprintf(stderr, "recv: %s\n", strerror(errno));
	return 1;
send_fail:
	fprintf(stderr, "send: %s\n", strerror(errno));
	return 1;
}
