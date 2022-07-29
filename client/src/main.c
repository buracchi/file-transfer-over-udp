#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <buracchi/common/argparser/argparser.h>
#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/networking/nproto/nproto_service_ipv4.h>
#include <buracchi/common/networking/tproto/tproto_service_tcp.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include <fts.h>
//#include "tproto/tproto_service_gbn.h"

enum client_operation {
	INVALID,
	EXIT,
	HELP,
	LIST,
	DOWNLOAD,
	UPLOAD
};

static enum client_operation get_client_operation(char const *buffer);
static int help();
static int get_list(fts_t fts, cmn_socket2_t socket);
static int download_file(fts_t fts, cmn_socket2_t socket, char const *filename);
static int upload_file(fts_t fts, cmn_socket2_t socket, char const *filename);
static int get_input(char **buffer);
static bool ask_to_override(char const *filename);

#define get_arg(buffer) ((buffer) + 4)

extern int main(int argc, const char *argv[]) {
	fts_t fts;
	cmn_socket2_t socket;
	cmn_argparser_t argparser;
	char *input_buff;
	char *url;
	try(argparser = cmn_argparser_init(), NULL, fail);
	try(cmn_argparser_set_description(argparser, "File transfer client."), 1, fail);
	try(cmn_argparser_add_argument(argparser, &url, CMN_ARGPARSER_ARGUMENT({ .name = "url", .help = "the file transfer server URL address" })), 1, fail);
	try(cmn_argparser_parse(argparser, argc, argv), 1, fail);
	try(cmn_argparser_destroy(argparser), 1, fail);
	try(fts = fts_init(), NULL, fail);
	try(printf("File Transfer Client\n\nType 'help' to get help.\n\n") < 0, true, fail);
	while (true) {
		enum client_operation operation;
		printf("FTC> ");
		try(get_input(&input_buff), 1, fail);
		operation = get_client_operation(input_buff);
		switch (operation) {
		case EXIT:
			free(input_buff);
			fts_destroy(fts);
			return EXIT_SUCCESS;
		case HELP:
			try(help() < 0, true, fail);
			break;
		default:
			try(socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), NULL, fail);
			try(cmn_socket2_connect(socket, url), 1, fail);
			switch (operation) {
			case LIST:
				try(get_list(fts, socket), 1, fail);
				break;
			case DOWNLOAD:
				try(download_file(fts, socket, get_arg(input_buff)), 1, fail);
				break;
			case UPLOAD:
				try(upload_file(fts, socket, get_arg(input_buff)),1, fail);
				break;
			default:
				try(printf("Invalid request\n") < 0, true, fail);
				break;
			}
			try(cmn_socket2_destroy(socket), 1, fail);
		}
		free(input_buff);
	}
fail:
	return EXIT_FAILURE;
}

static enum client_operation get_client_operation(char const *buffer) {
	if (streq(buffer, "exit")) {
		return EXIT;
	}
	else if (streq(buffer, "help")) {
		return HELP;
	}
	else if (!strncasecmp(buffer, "list", 4)) {
		return LIST;
	}
	else if (!strncasecmp(buffer, "get ", 4) && strlen(buffer) > 4) {
		return DOWNLOAD;
	}
	else if (!strncasecmp(buffer, "put ", 4) && strlen(buffer) > 4) {
		return UPLOAD;
	}
	return INVALID;
}

static int help() {
	return printf("\nSupported commands:\n\n\
			\r\t- list\t\t\tlist server available file\n\
			\r\t- get filename\t\tdownload a file from the server\n\
			\r\t- put filename\t\tupload a file to the server\n\
			\r\t- help\t\t\tprint this help\n\
			\r\t- exit\t\t\tclose client\n");
}

static int get_list(fts_t fts, cmn_socket2_t socket) {
	char *file_list;
	try(fts_get_file_list(fts, socket, &file_list) != FTS_ERROR_SUCCESS, true, fail);
	try(printf("%s\n", file_list) < 0, true, fail);
	free(file_list);
	return 0;
fail:
	return 1;
}

static int download_file(fts_t fts, cmn_socket2_t socket, char const *filename) {
	fts_state_t state = { 0 };
	enum fts_option option = DO_NOT_REPLACE;
download:
	switch (fts_download_file(fts, socket, filename, option, &state)) {
	case FTS_ERROR_SUCCESS:
		try(printf("File downloaded\n") < 0, true, fail);
		break;
	case FTS_ERROR_FILE_ALREADY_EXISTS:
		if (!ask_to_override(filename)) {
			option = REPLACE;
		}
		goto download;
	case FTS_ERROR_FILE_NOT_EXISTS:
		try(printf("Cannot find '%s': No such file\n", filename) < 0, true, fail);
		break;
	default:
		goto fail;
	}
	return 0;
fail:
	return 1;
}

static int upload_file(fts_t fts, cmn_socket2_t socket, char const *filename) {
	fts_state_t state = { 0 };
	enum fts_option option = DO_NOT_REPLACE;
upload:
	switch (fts_upload_file(fts, socket, filename, option, &state)) {
	case FTS_ERROR_SUCCESS:
		try(printf("File uploaded\n") < 0, true, fail);
		break;
	case FTS_ERROR_FILE_ALREADY_EXISTS:
		if (!ask_to_override(filename)) {
			option = REPLACE;
		}
		goto upload;
	case FTS_ERROR_FILE_NOT_EXISTS:
		try(printf("Cannot find '%s': No such file\n", filename) < 0, true, fail);
		break;
	default:
		goto fail;
	}
	return 0;
fail:
	return 1;
}

static int get_input(char **buffer) {
	try(scanf("%m[^\n]", buffer), -1, fail);
	try(getchar() != '\n' || !buffer, true, fail);
	return 0;
fail:
	return 1;
}

static inline bool ask_to_override(char const *filename) {
	char choice;
	printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
	choice = (char) getchar();
	fflush(stdin);
	return choice == 'y';
}
