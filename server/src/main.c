#include <signal.h>
#include <stdlib.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include "acceptor.h"
#include "cmdline.h"
#include "dispatcher.h"
#include "service_handler_factory.h"

static void event_accept(evutil_socket_t peer_acceptor, short event, void *arg);
static inline void set_log_level(int verbose_level);

extern int main(int argc, char *argv[argc + 1]) {
	cmdline_args_t args;
	dispatcher_t dispatcher = { 0 };
	struct acceptor acceptor = { 0 };
	service_handler_factory_t factory;
	dispatcher_event_t dispatcher_sigterm_event;
	dispatcher_event_t dispatcher_sigint_event;
    dispatcher_event_t accept_event;
	try(parse_cmdline_args(argc, (const char**)argv, &args), 1, argparser_fail);
	set_log_level(args.verbose_level);
	service_handler_factory_init(&factory, (struct service_parameters){ .directory = args.directory, .window_size = args.window_size, .timeout_duration = args.timeout_duration, .loss_probability = args.loss_probability });
	try(dispatcher_init(&dispatcher, args.thread_count), -1, dispatcher_init_fail);
	try(acceptor_init(&acceptor, args.listen_port), 1, acceptor_init_fail);
    try((accept_event = event_new(dispatcher.event_base, acceptor.peer_acceptor, EV_READ | EV_PERSIST, event_accept, &acceptor)), nullptr, event_new_fail);
    try(event_add(accept_event, nullptr), -1, event_add_fail);
	try(dispatcher_sigterm_event = dispatcher_stop_on_signal_event_add(&dispatcher, SIGTERM), nullptr, dispatcher_register_signal_handler_fail);
	try(dispatcher_sigint_event = dispatcher_stop_on_signal_event_add(&dispatcher, SIGINT), nullptr, dispatcher_register_signal_handler_fail);
	try(dispatcher_dispatch(&dispatcher), -1, dispatcher_dispatch_fail);
	cmn_logger_log_info("All events have been terminated.");
    try(dispatcher_event_remove(accept_event), -1, remove_event_fail);
	try(acceptor_destroy(&acceptor), 1, acceptor_destroy_fail);
	try(dispatcher_event_remove(dispatcher_sigterm_event), -1, remove_event_fail);
	try(dispatcher_event_remove(dispatcher_sigint_event), -1, remove_event_fail);
	dispatcher_destroy(&dispatcher);
	return EXIT_SUCCESS;
argparser_fail:
	cmn_logger_log_error("Can't parse CLI arguments.");
	return EXIT_FAILURE;
dispatcher_init_fail:
	cmn_logger_log_error("Can't initialize dispatcher.");
	return EXIT_FAILURE;
dispatcher_register_signal_handler_fail:
	return EXIT_FAILURE;
event_new_fail:
    cmn_logger_log_error("Can't create an accept event.");
    return EXIT_FAILURE;
event_add_fail:
    cmn_logger_log_error("Couldn't set accept event status to pending.");
    return EXIT_FAILURE;
acceptor_init_fail:
	cmn_logger_log_error("Can't initialize acceptor.");
	return EXIT_FAILURE;
dispatcher_dispatch_fail:
	cmn_logger_log_error("Event loop aborted for an unhandled backend error.");
	return EXIT_FAILURE;
remove_event_fail:
	cmn_logger_log_error("Error encountered when removing a dispatcher event.");
	return EXIT_FAILURE;
acceptor_destroy_fail:
	cmn_logger_log_error("Error encountered when destroying acceptor.");
	return EXIT_FAILURE;
}

static void event_accept(evutil_socket_t peer_acceptor, short event, void *arg) {
    struct acceptor *acceptor = arg;
    evutil_socket_t socket;
    struct acceptor_packet packet;
    acceptor_accept(acceptor, &socket, &packet);
    struct connection_details connection_details = {
            .peer_stream = socket,
            .request = (uint8_t *)packet.data,
            .request_size = packet.payload_size
    };
    //service_handler_factory_create(factory, dispatcher, connection_details);
    try(evutil_closesocket(socket), -1, fail);
fail:

}

static inline void set_log_level(int verbose_level) {
	switch (verbose_level) {
	case 1:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_WARN);
		break;
	case 2:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_INFO);
		break;
	case 3:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_DEBUG);
		break;
	default:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_ALL);
	}
}
