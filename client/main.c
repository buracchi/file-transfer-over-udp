#include <stdio.h>
#include <stdlib.h>

#include "socket2.h"
#include "utilities.h"

extern int main(int argc, char* argv[]) {
	socket2_t socket;
	socket = socket2_init(TCP, IPV4);
	socket2_ipv4_setaddr(socket, "127.0.0.1", 1234);
	socket2_connect(socket);
	socket2_send(socket, "Message\n");
	socket2_close(socket);
	socket2_destroy(socket);
	printf("Hello world!\n");
	return 0;
}
