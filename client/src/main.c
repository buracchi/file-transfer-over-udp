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
#include <ftcp.h>
//#include "tproto/tproto_service_gbn.h"

enum client_operation {
	INVALID, EXIT, HELP
};

static int get_input(char** buffer);
static enum client_operation get_client_operation(char* buffer);
static struct ftcp_operation get_ftcp_operation(char* buffer);
static int ftc_start(const char* url);
static int help();
static int require_list(cmn_socket2_t socket);
static int require_download(cmn_socket2_t socket, char* filename);
static int require_upload(cmn_socket2_t socket, char* filename);

#define get_arg(buffer) ((buffer) + 4)

extern int main(int argc, const char* argv[]) {
	char* url;
	cmn_argparser_t argparser;
	argparser = cmn_argparser_init();
	cmn_argparser_set_description(argparser, "File transfer client.");
	cmn_argparser_add_argument(argparser, &url, CMN_ARGPARSER_ARGUMENT({ .name = "url", .help = "the file transfer server URL address" }));
	cmn_argparser_parse(argparser, argc, argv);
	cmn_argparser_destroy(argparser);
	try(ftc_start(url), 1, fail);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}

static int ftc_start(const char* url) {
	cmn_socket2_t socket;
	char* buff;
	printf("File Transfer Client\n\nType 'help' to get help.\n\n");
	while (true) {
		printf("FTC> ");
		try(get_input(&buff), 1, fail);
		switch (get_client_operation(buff)) {
		case EXIT:
			free(buff);
			return 0;
		case HELP:
			help();
			break;
		default:
			//try(socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_gbn), NULL);
			try(socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), NULL, fail);
			try(cmn_socket2_connect(socket, url), 1, fail);
			switch (get_ftcp_operation(buff).value) {
			case FTCP_OPERATION_LIST_VALUE:
				require_list(socket);
				break;
			case FTCP_OPERATION_GET_VALUE:
				require_download(socket, get_arg(buff));
				break;
			case FTCP_OPERATION_PUT_VALUE:
				require_upload(socket, get_arg(buff));
				break;
			default:
				printf("Invalid request\n");
				break;
			}
			try(cmn_socket2_destroy(socket), 1, fail);
		}
		free(buff);
	}
fail:
	return 1;
}

static int help() {
	return printf("\nSupported commands:\n\n\
			\r\t- list\t\t\tlist server available file\n\
			\r\t- get filename\t\tdownload a file from the server\n\
			\r\t- put filename\t\tupload a file to the server\n\
			\r\t- help\t\t\tprint this help\n\
			\r\t- exit\t\t\tclose client\n");
}

static int get_input(char** buffer) {
	try(scanf("%m[^\n]", buffer), -1, fail);
	try(getchar() != '\n' || !buffer, true, fail);
	return 0;
fail:
	return 1;
}

static enum client_operation get_client_operation(char* buffer) {
	if (streq(buffer, "exit")) {
		return EXIT;
	}
	else if (streq(buffer, "help")) {
		return HELP;
	}
	return INVALID;
}

static struct ftcp_operation get_ftcp_operation(char* buffer) {
	if (!strncasecmp(buffer, "list", 4)) {
		return FTCP_OPERATION_LIST;
	}
	else if (!strncasecmp(buffer, "get ", 4) && strlen(buffer) > 4) {
		return FTCP_OPERATION_GET;
	}
	else if (!strncasecmp(buffer, "put ", 4) && strlen(buffer) > 4) {
		return FTCP_OPERATION_PUT;
	}
	return FTCP_OPERATION_INVALID;
}

static int require_list(cmn_socket2_t socket) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	char* filelist;
	ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_LIST, NULL, 0);
	cmn_socket2_send(socket, request, FTCP_PREAMBLE_PACKET_SIZE);
	cmn_socket2_recv(socket, response, FTCP_PREAMBLE_PACKET_SIZE);
	cmn_socket2_srecv(socket, &filelist);
	try(printf("%s\n", filelist) < 0, true, fail);
	free(filelist);
	return 0;
fail:
	return 1;
}

static int require_download(cmn_socket2_t socket, char* filename) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	char filename_buff[256] = { 0 };
	strcpy(filename_buff, filename);
	ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_GET, (uint8_t*)filename_buff, 0);
	cmn_socket2_send(socket, request, FTCP_PREAMBLE_PACKET_SIZE);
	cmn_socket2_recv(socket, response, FTCP_PREAMBLE_PACKET_SIZE);
	if (ftcp_preamble_packet_result(response) == FTCP_RESULT_FILE_EXIST_VALUE) {
		if (!access(filename, F_OK)) {
			printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
			char choice[2] = { (char)getchar(), 0 };
			try(getchar() != '\n' || !choice[0], true, fail);
			if (!streq(choice, "y")) {
				goto end;
			}
		}
		FILE* file;
		file = fopen(filename, "w");
		cmn_socket2_frecv(socket, file, (long)ftcp_preamble_packet_data_packet_length(response));
		try(printf("File downloaded\n") < 0, true, fail);
		fclose(file);
	}
	else {
		printf("Cannot find '%s': No such file\n", filename);
	}
end:
	return 0;
fail:
	return 1;
}

static int require_upload(cmn_socket2_t socket, char* filename) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	char filename_buff[256] = { 0 };
	strcpy(filename_buff, filename);
	FILE* file;
	uint64_t flen;
	file = fopen(filename, "r");
	fseek(file, 0L, SEEK_END);
	flen = ftell(file);
	fseek(file, 0L, SEEK_SET);
	ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_PUT, (uint8_t*)filename_buff, flen);
	cmn_socket2_send(socket, request, FTCP_PREAMBLE_PACKET_SIZE);
	cmn_socket2_recv(socket, response, FTCP_PREAMBLE_PACKET_SIZE);
	if (ftcp_preamble_packet_result(response) == FTCP_RESULT_FILE_EXIST_VALUE) {
		printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
		char choice[2] = { (char)getchar(), 0 };
		try(getchar() != '\n' || !choice[0], true, fail);
		if (!streq(choice, "y")) {
			goto end;
		}
	}
	cmn_socket2_fsend(socket, file);
	cmn_socket2_recv(socket, response, FTCP_PREAMBLE_PACKET_SIZE);
	try(printf("File uploaded\n") < 0, true, fail);
end:
	fclose(file);
	return 0;
fail:
	return 1;
}
