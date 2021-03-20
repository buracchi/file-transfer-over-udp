#include "ftc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

static int get_input(char** buffer);
static enum ftcp_operation get_operation(char* buffer);

static int require_list(socket2_t socket);
static int require_download(socket2_t socket, char* filename);
static int require_upload(socket2_t socket, char* filename);

#define get_arg(buffer) (buffer + 4)

extern int ftc_start(char* url) {
	socket2_t socket;
	char* buff;
	printf("File transfer client\nCommands: list, get, put\n");
	forever{
		printf(">");
		try(get_input(&buff), -1, fail);
		try(socket = socket2_init(TCP, IPV4), NULL);
		try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
		try(socket2_connect(socket), 1);
		switch (get_operation(buff)) {
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
		try(socket2_close(socket), 1);
		socket2_destroy(socket);
		free(buff);
	}
	return 0;
fail:
	return 1;
}

static int get_input(char** buffer) {
	try(scanf("%m[^\n]", buffer), -1, fail);
	try(getchar() != '\n' || !buffer, true, fail);
	return 0;
fail:
	return 1;
}

static enum ftcp_operation get_operation(char* buffer) {
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

static int require_list(socket2_t socket) {
	ftcp_pp_t request;
	ftcp_pp_t result;
	char* filelist;
	try(request = ftcp_pp_init(COMMAND, LIST, NULL, 0), NULL, fail);
	socket2_send(socket, request, ftcp_pp_size());
	try(result = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, result, ftcp_pp_size());
	socket2_srecv(socket, &filelist);
	try(printf("%s\n", filelist) < 0, true, fail);
	free(filelist);
	return 0;
fail:
	return 1;
}

static int require_download(socket2_t socket, char* filename) {
	ftcp_pp_t request;
	ftcp_pp_t result;
	char filename_buff[256] = { 0 };
	strcpy(filename_buff, filename);
	try(request = ftcp_pp_init(COMMAND, GET, filename_buff, 0), NULL, fail);
	socket2_send(socket, request, ftcp_pp_size());
	try(result = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, result, ftcp_pp_size());
	if (!access(filename, F_OK)) {
		printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
		char choise[2] = { getchar(), 0 };
		try(getchar() != '\n' || !choise[0], true, fail);
		if (!streq(choise, "y")) {
			return 0;
		}
	}
	FILE* file;
	file = fopen(filename, "w");
	socket2_frecv(socket, file, ftcp_get_dplen(result));
	try(printf("File downloaded\n") < 0, true, fail);
	fclose(file);
	return 0;
fail:
	return 1;
}

static int require_upload(socket2_t socket, char* filename) {
	ftcp_pp_t request;
	ftcp_pp_t result;
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
	try(result = malloc(ftcp_pp_size()), NULL, fail);
	socket2_recv(socket, result, ftcp_pp_size());
	int FILE_EXIST = !ftcp_get_arg(result)[0];
	if (FILE_EXIST) {
		printf("%s already exists. Do you want to replace it? [y/n]\n", filename);
		char choise[2] = { getchar(), 0 };
		try(getchar() != '\n' || !choise[0], true, fail);
		if (!streq(choise, "y")) {
			goto end;
		}
	}
	socket2_fsend(socket, file);
	socket2_recv(socket, result, ftcp_pp_size());
	try(printf("File uploaded\n") < 0, true, fail);
end:
	fclose(file);
	return 0;
fail:
	return 1;
}
