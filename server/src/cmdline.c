#include "cmdline.h"

#include <unistd.h>

#include <buracchi/argparser/argparser.h>
#include <buracchi/common/utilities/try.h>

int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t args[static 1]) {
    const char *cwd = getcwd(NULL, 0);
	argparser_t argparser;
	try(argparser = argparser_init(argc, argv), NULL, fail);
	argparser_set_description(argparser, "File transfer server.");
	try(argparser_add_argument(argparser, &(args->listen_port), { .flag = "p", .long_flag = "port", .default_value = "1234", .help = "specify the listening port number (the default value is \"1234\")" }), 1, fail);
	try(argparser_add_argument(argparser, &(args->directory), { .flag = "d", .long_flag = "directory", .default_value = cwd, .help = "specify the shared directory (the default value is the current working directory)" }), 1, fail);
	try(argparser_parse_args(argparser), 1, fail);
	argparser_destroy(argparser);
	return 0;
fail:
	return 1;
}
