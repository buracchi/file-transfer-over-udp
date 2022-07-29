#include <unistd.h>

#include <buracchi/common/argparser/argparser.h>
#include <buracchi/common/networking/request_handler.h>
#include <buracchi/common/networking/types/request_handler.h>
#include <buracchi/common/networking/communication_manager.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include <fts.h>
//#include "tproto/tproto_service_gbn.h"

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

struct fts_request_handler {
	struct cmn_request_handler handler;
	fts_t fts;
	char *directory;
};

static int destroy_request_handler(cmn_request_handler_t handler) {
	(void) handler;
	return 0;
}

static void handle_request(cmn_request_handler_t request_handler, cmn_socket2_t socket) {
	struct fts_request_handler *fts_request_handler = container_of(request_handler, struct fts_request_handler, handler);
	fts_handle_request(fts_request_handler->fts, socket, fts_request_handler->directory);
}

static struct cmn_request_handler_vtbl request_handler_ops_vtbl = {
	.destroy = &destroy_request_handler,
	.handle_request = &handle_request
};

extern int main(int argc, const char *argv[]) {
	fts_t fts;
	struct fts_request_handler fts_request_handler;
	cmn_communication_manager_t communication_manager;
	char *url;
	char *directory;
	cmn_argparser_t argparser;
	try(argparser = cmn_argparser_init(), NULL, fail);
	try(cmn_argparser_set_description(argparser, "File transfer server."), 1, fail);
	try(cmn_argparser_add_argument(argparser, &url, CMN_ARGPARSER_ARGUMENT({ .flag = "u", .long_flag = "url", .default_value = "0.0.0.0:1234", .help = "specify the listening port number (the default value is \"0.0.0.0:1234\")" })), 1, fail);
	try(cmn_argparser_add_argument(argparser, &directory, CMN_ARGPARSER_ARGUMENT({ .flag = "d", .long_flag = "directory", .default_value = getcwd(NULL,0), .help = "specify the shared directory (the default value is the current working directory)" })), 1, fail);
	try(cmn_argparser_parse(argparser, argc, argv), 1, fail);
	try(cmn_argparser_destroy(argparser), 1, fail);
	try(is_directory(directory), 0, fail);
	try(fts = fts_init(), NULL, fail);
	fts_request_handler.handler.__ops_vptr = &request_handler_ops_vtbl;
	fts_request_handler.fts = fts;
	fts_request_handler.directory = directory;
	try(communication_manager = cmn_communication_manager_init(4), NULL, fail);
	//cmn_communication_manager_set_tproto(communication_manager, tproto_service_gbn);
	try(cmn_communication_manager_start(communication_manager, url, &(fts_request_handler.handler)), 1, fail);
	try(cmn_communication_manager_destroy(communication_manager), 1, fail);
	try(fts_destroy(fts), 1, fail);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}
