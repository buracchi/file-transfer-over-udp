#include <unistd.h>

#include <buracchi/common/argparser/argparser.h>
#include <buracchi/common/networking/communication_manager.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include <fts.h>
//#include "tproto/tproto_service_gbn.h"

typedef struct fts_request_handler_info {
	fts_t fts;
	char *directory;
} *fts_request_handler_info_t;

static void handle_request(cmn_socket2_t socket, void* arg) {
	fts_request_handler_info_t info = (fts_request_handler_info_t)arg;
	fts_handle_request(info->fts, socket, info->directory);
}

extern int main(int argc, const char *argv[]) {
	fts_t fts;
	cmn_communication_manager_t communication_manager;
	struct fts_request_handler_info info;
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
	info.fts = fts;
	info.directory = directory;
	try(communication_manager = cmn_communication_manager_init(4), NULL, fail);
	//cmn_communication_manager_set_tproto(communication_manager, tproto_service_gbn);
	try(cmn_communication_manager_start(communication_manager, url, &handle_request, &info), 1, fail);
	try(cmn_communication_manager_destroy(communication_manager), 1, fail);
	try(fts_destroy(fts), 1, fail);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}
