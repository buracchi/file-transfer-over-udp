#include "service_handler.h"

#include <stdlib.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include <event2/event.h>

#include "inet_utils.h"

static void do_read(evutil_socket_t peer_stream, short events, void *arg);
static void do_write(evutil_socket_t peer_stream, short events, void *arg);

extern service_handler_t *service_handler_init(dispatcher_t *dispatcher, struct service_parameters parameters, struct connection_details connection_details) {
	service_handler_t *service_handler;
	struct event *read_event;
	struct event *write_event;
	try(service_handler = malloc(sizeof *service_handler), NULL, fail);
	service_handler->directory = parameters.directory;
	service_handler->window_size = parameters.window_size;
	service_handler->timeout_duration = parameters.timeout_duration;
	service_handler->loss_probability = parameters.loss_probability;
	try(read_event = event_new(dispatcher->event_base, connection_details.peer_stream, EV_READ | EV_PERSIST, do_read, service_handler), NULL, read_event_init_fail);
	try(write_event = event_new(dispatcher->event_base, connection_details.peer_stream, EV_WRITE | EV_PERSIST, do_write, service_handler), NULL, write_event_init_fail);
	memcpy(&(service_handler->state), &(struct service_handler_state){ .buffer = "MSG", .buffer_used = 0, .n_written = 0, .write_upto = sizeof "MSG", .read_event = read_event, .write_event = write_event }, sizeof service_handler->state);
	try(event_add(service_handler->state.write_event, NULL), -1, write_event_add_fail);
	return service_handler;
write_event_add_fail:
write_event_init_fail:
	event_free(read_event);
read_event_init_fail:
	free(service_handler);
fail:
	cmn_logger_log_error("Can't initialize service handler");
	return NULL;
}

extern void service_handler_destroy(service_handler_t *service_handler) {
	event_free(service_handler->state.read_event);
	event_free(service_handler->state.write_event);
	free(service_handler);
}

extern char *service_handler_error_to_string(enum service_handler_error error) {
	switch (error) {
	case SERVICE_HANDLER_SUCCESS:
		return "Success";
	case SERVICE_HANDLER_ENOMEM:
		return "Out of memory.";
	default:
		return "Unknown error code.";
	}
}

static void do_read(evutil_socket_t peer_stream, short events, void *arg) {
	service_handler_t *service_handler = arg;
	ssize_t result;
	char rbuff[1024];
	cmn_logger_log_debug("Waiting for client response.");
	while (true) {
		try((result = recv(peer_stream, rbuff, sizeof rbuff - 1, 0)) == -1, true, recv_fail);
		cmn_logger_log_debug("Received %zd bytes, packet content is \"%s\"", result, rbuff);
		cmn_logger_log_debug("Closing connection");
		evutil_closesocket(peer_stream);
		service_handler_destroy(service_handler);
		return;
	}
recv_fail:
	if (evutil_socket_geterror(peer_stream) == EAGAIN) {
		return;
	}
	cmn_logger_log_error("recv: %s\n", evutil_socket_error_to_string(evutil_socket_geterror(peer_stream)));
	service_handler_destroy(service_handler);
}

static void do_write(evutil_socket_t peer_stream, short events, void *arg) {
	service_handler_t *service_handler = arg;
	ssize_t result;
	cmn_logger_log_debug("Replying to client.");
	while (service_handler->state.n_written < service_handler->state.write_upto) {
		result = send(peer_stream, service_handler->state.buffer + service_handler->state.n_written, service_handler->state.write_upto - service_handler->state.n_written, 0);
		if (result < 0) {
			if (evutil_socket_geterror(peer_stream) == EAGAIN) {
				return;
			}
			service_handler_destroy(service_handler);
			return;
		}
		service_handler->state.n_written += result;
	}
	cmn_logger_log_debug("Message sent.");
	if (service_handler->state.n_written == service_handler->state.buffer_used) {
		service_handler->state.n_written = 1;
		service_handler->state.write_upto = 1;
		service_handler->state.buffer_used = 1;
	}
	event_del(service_handler->state.write_event);
	event_add(service_handler->state.read_event, NULL);
}
