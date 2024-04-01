#include "cmdline.h"

#include <unistd.h>

#include <buracchi/argparser/argparser.h>
#include <buracchi/common/utilities/try.h>

// Parses command-line arguments and fills in the fields of a cmdline_args struct
// Returns 0 on success, 1 on failure
int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t *args) {
	argparser_t parser;
	argparser_t list_parser;
	argparser_t get_parser;
	argparser_t put_parser;
	try(parser = argparser_init(argc, argv), NULL, fail);
	argparser_set_description(parser, "File transfer client.");
	try(argparser_add_argument(parser, &args->host, { .name = "host", .help = "the file transfer server address" }), 1, fail);
	try(argparser_add_argument(parser, &args->port, { .flag = "p", .long_flag = "port", .default_value = "1234", .help = "specify the listening port number (the default value is \"1234\")" }), 1, fail);
	argparser_set_subparsers_options(parser, (struct argparser_subparsers_options){ .required = true, .title = "commands" });
	try(list_parser = argparser_add_subparser(parser, &args->command, "list", "List available file on the remote server"), NULL, fail);
	try(get_parser = argparser_add_subparser(parser, &args->command, "get", "Download a file from the remote server"), NULL, fail);
	try(argparser_add_argument(get_parser, &args->filename, { .name = "filename", .help = "The file to download" }), 1, fail);
	try(put_parser = argparser_add_subparser(parser, &args->command, "put", "Upload a file to the remote server"), NULL, fail);
	try(argparser_add_argument(put_parser, &args->filename, { .name = "filename", .help = "The file to upload" }), 1, fail);
	try(argparser_parse_args(parser), 1, fail);
	argparser_destroy(parser);
	return 0;
fail:
	return 1;
}
