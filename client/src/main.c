#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/util.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include "cmdline.h"
#include "connector.h"
#include "service_handler.h"

void set_log_level(int verbose_level);

int main(int argc, char *argv[argc + 1]) {
	cmdline_args_t args;
	evutil_socket_t sockfd = EVUTIL_INVALID_SOCKET;
	try(parse_cmdline_args(argc, (const char**)argv, &args), 1, argparser_fail);
	set_log_level(args.verbose_level);
	try(connector_connect(&sockfd, args.host, args.port), 1, fail);
	try(service_handler_handle_request(
		    &(service_handler_t){
			    .command = args.command,
			    .filename = args.filename,
			    .window_size = args.window_size,
			    .timeout_duration = args.timeout_duration,
			    .adaptive_timeout = args.adaptive_timeout,
			    .loss_probability = args.loss_probability },
		    sockfd),
	    1,
	    fail);
	cmn_logger_log_info("Closing connection.");
	try(evutil_closesocket(sockfd), -1, close_fail);
	return EXIT_SUCCESS;
argparser_fail:
	cmn_logger_log_error("Can't parse CLI arguments.");
	return EXIT_FAILURE;
close_fail:
	cmn_logger_log_error("close: %s\n.", strerror(errno));
	return EXIT_FAILURE;
fail:
	return EXIT_FAILURE;
}

void set_log_level(int verbose_level) {
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
