#include <unistd.h>

#include "argparser.h"
#include "map.h"
#include "ft_handler.h"
#include "communication_manager.h"
#include "tproto/tproto_service_gbn.h"
#include "utilities.h"
#include "try.h"

extern int main(int argc, const char *argv[]) {
    cmn_argparser_t argparser;
    cmn_map_t option_map;
    struct cmn_argparser_argument args[] = {
            {.flag = "u", .long_flag="url",         .default_value="0.0.0.0:1234",          .help="specify the listening port number (the default value is \"0.0.0.0:1234\")"},
            {.flag = "d", .long_flag="directory",   .default_value=getcwd(NULL,0), .help="specify the shared directory (the default value is the current working directory)"},
    };
    argparser = cmn_argparser_init(argv[0], "File transfer server.");
    cmn_argparser_set_arguments(argparser, args, 2);
    option_map = cmn_argparser_parse(argparser, argc, argv);
    char *url;
    char *directory;
    cmn_map_at(option_map, (void *) "u", (void **) &url);
    cmn_map_at(option_map, (void *) "d", (void **) &directory);
    try(is_directory(directory), 0);
    cmn_argparser_destroy(argparser);
    cmn_request_handler_t ft_handler;
    cmn_communication_manager_t communication_manager;
    try(ft_handler = (cmn_request_handler_t) ft_handler_init(directory), NULL);
    try(communication_manager = cmn_communication_manager_init(4), NULL);
    //cmn_communication_manager_set_tproto(communication_manager, tproto_service_gbn);
    try(cmn_communication_manager_start(communication_manager, url, ft_handler), 1);
    cmn_request_handler_destroy(ft_handler);
    cmn_communication_manager_destroy(communication_manager);
    return 0;
}
