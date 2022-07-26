#include "ft_service.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __unix__
#include <unistd.h>
#endif

#include <buracchi/common/concurrency/rwfslock.h>
#include <buracchi/common/networking/types/request_handler.h>
#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include "ftcp.h"

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

struct ft_service {
	char* base_dir;
	cmn_rwfslock_t rwfslock;
	struct cmn_request_handler request_handler;
};

static int destroy_request_handler(cmn_request_handler_t handler) { return 0; }
static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket);
static char* get_file_list(ft_service_t ft_service);
static void send_file_list(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_get_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_put_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_invalid_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);

static struct cmn_request_handler_vtbl request_handler_ops_vtbl = {
		.destroy = destroy_request_handler,
		.handle_request = handle_request
};

extern ft_service_t ft_service_init(const char* base_dir) {
	ft_service_t ft_service;
	try(ft_service = malloc(sizeof * ft_service), NULL, fail);
	try(ft_service->base_dir = malloc(strlen(base_dir) + 1), NULL, fail2);
	try(ft_service->rwfslock = cmn_rwfslock_init(), NULL, fail3);
	strcpy(ft_service->base_dir, base_dir);
	ft_service->request_handler.__ops_vptr = &request_handler_ops_vtbl;
	return ft_service;
fail3:
	free(ft_service->base_dir);
fail2:
	free(ft_service);
fail:
	return NULL;
}

extern int ft_service_destroy(ft_service_t ft_service) {
	cmn_rwfslock_destroy(ft_service->rwfslock);
	free(ft_service->base_dir);
	free(ft_service);
	return 0;
}

extern cmn_request_handler_t ft_service_get_request_handler(ft_service_t ft_service) {
	return &(ft_service->request_handler);
}

static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket) {
	ft_service_t ft_service = container_of(handler, struct ft_service, request_handler);
	ftcp_preamble_packet_t request;
	try(cmn_socket2_recv(socket, request, FTCP_PREAMBLE_PACKET_SIZE), -1, fail);
	if (ftcp_preamble_packet_type(request) == FTCP_TYPE_COMMAND_VALUE) {
		switch (ftcp_preamble_packet_operation(request)) {
		case FTCP_OPERATION_LIST_VALUE:
			send_file_list(ft_service, socket, request);
			break;
		case FTCP_OPERATION_GET_VALUE:
			handle_get_request(ft_service, socket, request);
			break;
		case FTCP_OPERATION_PUT_VALUE:
			handle_put_request(ft_service, socket, request);
			break;
		default:
			handle_invalid_request(ft_service, socket, request);
		}
	}
	else {
		handle_invalid_request(ft_service, socket, request);
	}
fail:
	return;
}

static void send_file_list(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	char* file_list = get_file_list(ft_service);
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, strlen(file_list));
	try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail);
	try(cmn_socket2_ssend(socket, file_list), -1, fail);
	free(file_list);
fail:
	return;
}

static void handle_get_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	char* frpath;
	char* fpath;
	FILE* file;
	uint64_t flen;
	frpath = (char*) ftcp_preamble_packet_arg(request);
	if (strstr(frpath, "/")) {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_INVALID_ARGUMENT, NULL, 0);
		cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
		return;
	}
	asprintf(&fpath, "%s/%s", ft_service->base_dir, frpath);
	if (!access(fpath, F_OK)) {
		try(cmn_rwfslock_rdlock(ft_service->rwfslock, fpath), 1, fail);
		file = fopen(fpath, "r");
		fseek(file, 0L, SEEK_END);
		flen = ftell(file);
		fseek(file, 0L, SEEK_SET);
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_EXIST, frpath, flen);
		cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
		cmn_socket2_fsend(socket, file);
		fclose(file);
		try(cmn_rwfslock_unlock(ft_service->rwfslock, fpath), 1, fail);
	}
	else {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_NOT_EXIST, NULL, 0);
		cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
	}
	free(fpath);
fail:
	return;
}

static void handle_put_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	FILE* file;
	char* fpath;
	struct ftcp_result result;
	asprintf(&fpath, "%s/%s", ft_service->base_dir, ftcp_preamble_packet_arg(request));
	result.value = access(fpath, F_OK) ? FTCP_RESULT_FILE_NOT_EXIST_VALUE : FTCP_RESULT_FILE_EXIST_VALUE;
	try(cmn_rwfslock_wrlock(ft_service->rwfslock, fpath), 1, fail);
	file = fopen(fpath, "w");
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, result, NULL, 0);
	cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
	cmn_socket2_frecv(socket, file, (long) ftcp_preamble_packet_data_packet_length(request));
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, 0);
	cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
	fclose(file);
	try(cmn_rwfslock_unlock(ft_service->rwfslock, fpath), 1, fail);
	free(fpath);
fail:
	return;
}

static void handle_invalid_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_ERROR, NULL, 0);
	cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
}

static char* get_file_list(ft_service_t ft_service) {
	static const char* list_command;
	try(asprintf((char**) &list_command, "ls %s -p | grep -v /", ft_service->base_dir), -1, fail);
	FILE* pipe;
	char* filelist;
	pipe = popen(list_command, "r");
	fscanf(pipe, "%m[\x01-\xFF-]", &filelist);
	pclose(pipe);
	return filelist;
fail:
	return NULL;
}
