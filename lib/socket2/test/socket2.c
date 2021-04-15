#include "socket2.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "tproto/tproto_tcp.h"
#include "nproto/nproto_ipv4.h"
#include "try.h"
#include "utilities.h"

static void before_test();
static int client();
static int server();

char* message = "MESSAGE";
struct tproto* tproto;
struct nproto* nproto;

extern int main(int argc, char** argv) {
	pid_t pid;
	int child_stat;
	before_test();
	try(pid = fork(), -1, fail);
	if (!pid) {
		return client();
	}
	server();
	try(wait(&child_stat), -1, fail);
	try(WIFEXITED(child_stat), EXIT_FAILURE, fail);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}

static void before_test() {
	struct tproto_tcp tcp;
	struct nproto_ipv4 ipv4;
	tproto_tcp_init(&tcp);
	nproto_ipv4_init(&ipv4);
	tproto = &tcp.super.tproto;
	nproto = &ipv4.super.nproto;
}

int client() {
	struct socket2* socket;
	char* buff = malloc(strlen(message) + 1);
	memset(buff, 0, strlen(message) + 1);
	try(socket = new(socket2, tproto, nproto), NULL, error);
	try(socket2_connect(socket, "127.0.0.1:1234"), 1, error);
	try(socket2_recv(socket, message, strlen(message)), -1, error);
	try(socket2_destroy(socket), 1, error);
	try(strcmp(message, buff) == 0, false, error);
	delete(socket);
	return EXIT_SUCCESS;
error:
	return EXIT_FAILURE;
}

int server() {
	struct socket2* server;
	struct socket2* client;
	try(server = new(socket2, tproto, nproto), NULL, error);
	try(socket2_listen(server, "0.0.0.0:1234", 1024),1, error);
	try(client = socket2_accept(server), NULL, error);
	try(socket2_send(client, message, strlen(message)), -1, error);
	delete(server);
	delete(client);
error:
	return EXIT_FAILURE;
}
