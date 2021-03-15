#include "fts.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>

#include "socket2.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

static void* foo(void* arg) {
	socket2_t socket;
	char *buff;
	int port = (long) arg;
	asprintf(&buff, "message sent via tcp from thread #%lu", pthread_self());
	socket = socket2_init(TCP, IPV4);
	socket2_ipv4_setaddr(socket, "127.0.0.1", port);
	socket2_connect(socket);
	socket2_send(socket, buff);
	free(buff);
	socket2_close(socket);
	socket2_destroy(socket);
}

extern int fts_start(int port, char* pathname) {
	socket2_t socket;
	tpool_t tpool = tpool_create(8);
    socket = socket2_init(TCP, IPV4);
    socket2_ipv4_setaddr(socket, "127.0.0.1", port);
	socket2_listen(socket, 256);
    printf("Server started.\n");
	for (int i = 0; i < 20; i++) {
		tpool_add_work(tpool, foo, (void*)(long)port);
	}
	struct pollfd pollfds[1];
	pollfds[0].fd = socket2_get_fd(socket);
	pollfds[0].events = POLLIN;
	while (1) {
		try(poll(&pollfds, 1, 3000), -1, exit);
		if (pollfds[0].revents & POLLIN) {
			socket2_t accepted;
			char* buff;
			accepted = socket2_accept(socket);
			socket2_recv(accepted, &buff);
			printf("%s\n", buff);
			free(buff);
			socket2_close(accepted);
			socket2_destroy(accepted);
		}
	}
exit:
	tpool_wait(tpool);
	tpool_destroy(tpool);
	socket2_close(socket);
	socket2_destroy(socket);
	return 0;
}
