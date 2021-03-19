#include "ftc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

extern int ftc_start(char* url) {
	socket2_t socket;
	printf("File transfer client, type 'list' and press enter...\n");
	forever {
		ftcp_pp_t request;
		ftcp_pp_t result = malloc(ftcp_pp_size());
		char* buff;
		scanf("%m[^\n]", &buff);
		if (getchar() != '\n' || !buff) {
			free(result);
			return 1;
		}
		try(socket = socket2_init(TCP, IPV4), NULL);
		try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
		try(socket2_connect(socket), 1);
		if (!strncasecmp(buff, "list", 4)) {
			char* filelist;
			request = ftcp_pp_init(COMMAND, LIST, NULL, 0);
			socket2_send(socket, request, ftcp_pp_size());
			socket2_recv(socket, result, ftcp_pp_size());
			socket2_srecv(socket, &filelist);
			printf("%s\n", filelist);
			free(filelist);
		}
		else if (!strncasecmp(buff, "get ", 4) && strlen(buff) > 4) {
			char filename[256] = { 0 };
			strcpy(filename, buff + 4);
			request = ftcp_pp_init(COMMAND, GET, filename, 0);
			socket2_send(socket, request, ftcp_pp_size());
			socket2_recv(socket, result, ftcp_pp_size());
			FILE* file;
			file = fopen(filename, "w");
			socket2_frecv(socket, file, ftcp_get_dplen(result));
			fclose(file);
			printf("file downloaded\n");
		}
		else if (!strncasecmp(buff, "put ", 4) && strlen(buff) > 4) {
			char filename[256] = { 0 };
			strcpy(filename, buff + 4);
			FILE* file;
			uint64_t flen;
			file = fopen(filename, "r");
			fseek(file, 0L, SEEK_END);
			flen = ftell(file);
			fseek(file, 0L, SEEK_SET);
			request = ftcp_pp_init(COMMAND, PUT, filename, flen);
			socket2_send(socket, request, ftcp_pp_size());
			socket2_fsend(socket, file);
			fclose(file);
			printf("file uploaded\n");
		}
		else {
			request = ftcp_pp_init(COMMAND, INVALID_OPERATION, NULL, 0);
			socket2_send(socket, request, ftcp_pp_size());
		}
		free(buff);
		try(socket2_close(socket), 1);
		socket2_destroy(socket);
		free(request);
		free(result);
	}
	return 0;
}
