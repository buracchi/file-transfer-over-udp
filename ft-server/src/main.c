#include <unistd.h>

#include <buracchi/common/argparser/argparser.h>
#include <buracchi/common/networking/communication_manager.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include "ft_service.h"
//#include "tproto/tproto_service_gbn.h"

extern int main(int argc, const char* argv[]) {
	ft_service_t ft_service;
	cmn_communication_manager_t communication_manager;
	cmn_request_handler_t request_handler;
	char* url;
	char* directory;
	cmn_argparser_t argparser;
	try(argparser = cmn_argparser_init(), NULL, fail);
	try(cmn_argparser_set_description(argparser, "File transfer server."), 1, fail);
	try(cmn_argparser_add_argument(argparser, &url, CMN_ARGPARSER_ARGUMENT({ .flag = "u", .long_flag = "url", .default_value = "0.0.0.0:1234", .help = "specify the listening port number (the default value is \"0.0.0.0:1234\")" })), 1, fail);
	try(cmn_argparser_add_argument(argparser, &directory, CMN_ARGPARSER_ARGUMENT({ .flag = "d", .long_flag = "directory", .default_value = getcwd(NULL,0), .help = "specify the shared directory (the default value is the current working directory)" })), 1, fail);
	try(cmn_argparser_parse(argparser, argc, argv), 1, fail);
	try(cmn_argparser_destroy(argparser), 1, fail);
	try(is_directory(directory), 0, fail);
	try(ft_service = ft_service_init(directory), NULL, fail);
	try(request_handler = ft_service_get_request_handler(ft_service), NULL, fail);
	try(communication_manager = cmn_communication_manager_init(4), NULL, fail);
	//cmn_communication_manager_set_tproto(communication_manager, tproto_service_gbn);
	try(cmn_communication_manager_start(communication_manager, url, request_handler), 1, fail);
	try(cmn_communication_manager_destroy(communication_manager), 1, fail);
	try(ft_service_destroy(ft_service), 1, fail);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}
