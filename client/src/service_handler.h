#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <event2/util.h>

typedef struct service_handler {
	char const *command;     // command to send to the server (e.g. "list", "get", "put")
	char *filename;          // file to send or receive (used with the "get" and "put" commands)
	uint32_t window_size;    // size of the dispatch window to use for the Go-Back N protocol
	long timeout_duration;   // duration of the timeout to use for the Go-Back N protocol, in milliseconds
	bool adaptive_timeout;   // flag to use an adaptive timeout calculated dynamically based on network delays
	double loss_probability; // probability of message loss to simulate
} service_handler_t;

int service_handler_handle_request(service_handler_t* service_handler, evutil_socket_t sockfd);
