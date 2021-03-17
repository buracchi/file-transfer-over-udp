#include "ftc.h"

#include <stdio.h>
#include <stdlib.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

extern int ftc_start(char* url) {
	ftcp_message_t request;
	ftcp_message_t result;
	socket2_t socket;
	printf("File transfer client, type 'list' and press enter...\n");
	forever {
		char* buff;
		scanf("%ms", &buff);
		if (!strcasecmp(buff, "list")) {
			request = ftcp_message_init(COMMAND, LIST, NULL, 0, NULL);
			free(buff);
			try(socket = socket2_init(TCP, IPV4), NULL);
			try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
			try(socket2_connect(socket), 1);
			try(socket2_send(socket, request), -1);
			try(socket2_recv(socket, (char**)&result), -1);
			try(socket2_close(socket), 1);
			socket2_destroy(socket);
			printf("%s\n", ftcp_get_filename(result));
			free(request);
			free(result);
		}
	}
	return 0;
}
