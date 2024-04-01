#pragma once

#include <event2/util.h>

#include "dispatcher.h"

enum service_handler_error {
	SERVICE_HANDLER_SUCCESS = 0,
	SERVICE_HANDLER_ENOMEM = 1
};

struct service_handler_state {
	char buffer[512];
	size_t buffer_used;
	size_t n_written;
	size_t write_upto;
	struct event *read_event;
	struct event *write_event;
};

typedef struct service_handler {
	char *directory;         // file root directory
	uint32_t window_size;    // size of the dispatch window
	long timeout_duration;   // duration of the timeout in milliseconds
	double loss_probability; // probability of packet loss
	struct service_handler_state state;
} service_handler_t;

struct service_parameters {
	char *directory;         // file root directory
	uint32_t window_size;    // size of the dispatch window
	long timeout_duration;   // duration of the timeout in milliseconds
	double loss_probability; // probability of packet loss
};

struct connection_details {
	evutil_socket_t peer_stream;
	uint8_t *request;
	size_t request_size;
};

extern service_handler_t *service_handler_init(dispatcher_t *dispatcher, struct service_parameters parameters, struct connection_details connection_details);

extern void service_handler_destroy(service_handler_t *service_handler);

extern char *service_handler_error_to_string(enum service_handler_error error);
