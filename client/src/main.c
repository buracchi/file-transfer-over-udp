#include <stdio.h>
#include <stdlib.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

extern int main(int argc, char* argv[]) {
	char message[3] = {0};
	char* reply;
	socket2_t socket;
	message[0] = COMMAND;
	message[1] = LIST;
	try(socket = socket2_init(TCP, IPV4), NULL);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
	try(socket2_connect(socket), 1);
	try(socket2_send(socket, message), -1);
	try(socket2_recv(socket, &reply), -1);
	try(socket2_close(socket), 1);
	socket2_destroy(socket);
	printf("%s\n", reply);
	free(reply);
	return 0;
}
