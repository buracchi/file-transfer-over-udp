#include <stdio.h>
#include <stdlib.h>

#include "socket2.h"
#include "try.h"
#include "utilities.h"

extern int main(int argc, char* argv[]) {
	socket2_t socket;
	try(socket = socket2_init(TCP, IPV4), NULL);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
	try(socket2_connect(socket), 1);
	try(socket2_send(socket, "Message\n"), -1);
	try(socket2_close(socket), 1);
	socket2_destroy(socket);
	printf("Hello world!\n");
	return 0;
}
