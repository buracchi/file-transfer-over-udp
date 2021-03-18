#include "ftc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

static const char* ftcp_get_type_str(ftcp_message_t message) {
	enum ftcp_type type = ftcp_get_type(message);
	switch (type) {
	case COMMAND:
		return "COMMAND";
	case RESPONSE:
		return "RESPONSE";
	default:
		return "INVALID";
	}
}

static const char* ftcp_get_operation_str(ftcp_message_t message) {
	enum ftcp_type type = ftcp_get_type(message);
	enum ftcp_operation operation = ftcp_get_operation(message);
	switch (type) {
	case COMMAND:
		switch (operation) {
		case LIST:
			return "LIST";
		case GET:
			return "GET";
		case PUT:
			return "PUT";
		default:
			return "INVALID";
		}
	case RESPONSE:
		switch (operation) {
		case SUCCESS:
			return "SUCCESS";
		case ERROR:
			return "ERROR";
		default:
			return "INVALID";
		}
	default:
		return "INVALID";
	}
}

extern int ftc_start(char* url) {
	ftcp_message_t request;
	ftcp_message_t result;
	socket2_t socket;
	printf("File transfer client, type 'list' and press enter...\n");
	forever {
		char* buff;
		scanf("%m[^\n]", &buff);
		if (getchar() != '\n') {
			return 1;
		}
		if (!strncasecmp(buff, "list", 4)) {
			request = ftcp_message_init(COMMAND, LIST, NULL, 0, NULL);
		}
		else if (!strncasecmp(buff, "get ", 4) && strlen(buff) > 4) {
			request = ftcp_message_init(COMMAND, GET, buff + 4, 0, NULL);
		}
		else if (!strncasecmp(buff, "put ", 4) && strlen(buff) > 4) {
			FILE* file;
			uint64_t length;
			uint8_t* content;
			file = fopen(buff + 4, "r");
			fseek(file, 0L, SEEK_END);
			length = ftell(file);
			fseek(file, 0L, SEEK_SET);
			content = malloc(sizeof * content * length);
			fread(content, 1, length, file);
			fclose(file);
			request = ftcp_message_init(COMMAND, PUT, buff + 4, length, content);
			free(content);
		}
		else {
			request = ftcp_message_init(COMMAND, INVALID_OPERATION, NULL, 0, NULL);
		}
		free(buff);
		try(socket = socket2_init(TCP, IPV4), NULL);
		try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
		try(socket2_connect(socket), 1);
		try(socket2_send(socket, request, ftcp_message_length(request)), -1);
		try(socket2_srecv(socket, (char**)&result), -1);
		try(socket2_close(socket), 1);
		socket2_destroy(socket);
		result = realloc(result, 267);
		result[266] = 0;
		printf(
			"\n+----------------FTCP Packet----------------+\n\
			 \r| Type: %s\n\
			 \r| Operation/Result: %s\n\
			 \r| Filename/Message: %s\n\
			 \r| Length: %lu\n\
			 \r| Content/Arg: %s\n\
			 \r+-------------------------------------------+\n",
			ftcp_get_type_str(result),
			ftcp_get_operation_str(result),
			ftcp_get_filename(result),
			ftcp_get_file_length(result),
			ftcp_get_file_content(result)
		);
		free(request);
		free(result);
	}
	return 0;
}
