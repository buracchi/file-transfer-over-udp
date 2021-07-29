#include "ftc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "ftcp.h"
#include "socket2.h"
#include "nproto/nproto_ipv4.h"
#include "tproto/tproto_tcp.h"
#include "try.h"
#include "utilities.h"

enum client_operation { INVALID, EXIT, HELP };

static int get_input(char** buffer);
static enum client_operation get_client_operation(char* buffer);
static enum ftcp_operation get_ftcp_operation(char* buffer);

static int help();
static int require_list(struct socket2* socket);
static int require_download(struct socket2* socket, char* filename);
static int require_upload(struct socket2* socket, char* filename);

#define get_arg(buffer) (buffer + 4)

extern int ftc_start(const char* url) {
	struct socket2* socket;
	struct nproto_ipv4 ipv4;
	struct tproto_tcp tcp;
	char* buff;
	nproto_ipv4_init(&ipv4);
	tproto_tcp_init(&tcp);
	printf("File Transfer Client\n\nType 'help' to get help.\n\n");
	for (;;) {
		printf("FTC> ");
		try(get_input(&buff), -1, fail);
		switch (get_client_operation(buff)) {
		case EXIT:
			free(buff);
			return 0;
		case HELP:
			help();
			break;
		default:
			try(socket = new(socket2, &tcp.super.tproto, &ipv4.super.nproto, true), NULL);
			try(socket2_connect(socket, url), 1);
			switch (get_ftcp_operation(buff)) {
			case LIST:
				require_list(socket);
				break;
			case GET:
				require_download(socket, get_arg(buff));
				break;
			case PUT:
				require_upload(socket, get_arg(buff));
				break;
			default:
				printf("Invalid request\n");
				break;
			}
			try(delete(socket), 1);
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

static enum ftcp_operation get_ftcp_operation(char* buffer) {
	if (!strncasecmp(buffer, "list", 4)) {
		return LIST;
	}
	else if (!strncasecmp(buffer, "get ", 4) && strlen(buffer) > 4) {
		return GET;
	}
	else if (!strncasecmp(buffer, "put ", 4) && strlen(buffer) > 4) {
		return PUT;
	}
	return INVALID_OPERATION;
}

static int require_list(struct socket2* socket) {
	ftcp_pp_t request;
	ftcp_pp_t response;
	char* filelist;
	try(request = ftcp_pp_init(COMMAND, LIST, NULL, 0), NULL, fail);
	socket2_send(socket, request, ftcp_pp_size());
	free(request);
	try(response = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, response, ftcp_pp_size());
	free(response);
	socket2_srecv(socket, &filelist);
	try(printf("%s\n", filelist) < 0, true, fail);
	free(filelist);
	return 0;
fail:
	return 1;
}

static int require_download(struct socket2* socket, char* filename) {
	ftcp_pp_t request;
	ftcp_pp_t response;
	char filename_buff[256] = { 0 };
	strcpy(filename_buff, filename);
	try(request = ftcp_pp_init(COMMAND, GET, filename_buff, 0), NULL, fail);
	socket2_send(socket, request, ftcp_pp_size());
	free(request);
	try(response = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, response, ftcp_pp_size());
	if (ftcp_get_result(response) == FILE_EXIST) {
		if (!access(filename, F_OK)) {
			printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
			char choise[2] = { getchar(), 0 };
			try(getchar() != '\n' || !choise[0], true, fail);
			if (!streq(choise, "y")) {
				goto end;
			}
		}
		FILE* file;
		file = fopen(filename, "w");
		socket2_frecv(socket, file, ftcp_get_dplen(response));
		try(printf("File downloaded\n") < 0, true, fail);
		fclose(file);
	}
	else {
		printf("Cannot find '%s': No such file\n", filename);
	}
end:
	free(response);
	return 0;
fail:
	return 1;
}

static int require_upload(struct socket2* socket, char* filename) {
	ftcp_pp_t request;
	ftcp_pp_t response;
	char filename_buff[256] = { 0 };
	strcpy(filename_buff, filename);
	FILE* file;
	uint64_t flen;
	file = fopen(filename, "r");
	fseek(file, 0L, SEEK_END);
	flen = ftell(file);
	fseek(file, 0L, SEEK_SET);
	try(request = ftcp_pp_init(COMMAND, PUT, filename_buff, flen), NULL, fail);
	socket2_send(socket, request, ftcp_pp_size());
	free(request);
	try(response = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, response, ftcp_pp_size());
	if (ftcp_get_result(response) == FILE_EXIST) {
		printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
		char choise[2] = { getchar(), 0 };
		try(getchar() != '\n' || !choise[0], true, fail);
		if (!streq(choise, "y")) {
			goto end;
		}
	}
	socket2_fsend(socket, file);
	socket2_recv(socket, response, ftcp_pp_size());
	try(printf("File uploaded\n") < 0, true, fail);
end:
	fclose(file);
	free(response);
	return 0;
fail:
	return 1;
}
