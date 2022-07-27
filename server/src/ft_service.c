#include "ft_service.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef __unix__
#include <unistd.h>
#endif

#include <buracchi/common/concurrency/rwfslock.h>
#include <buracchi/common/logger/logger.h>
#include <buracchi/common/networking/types/request_handler.h>
#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/utilities.h>
#include <buracchi/common/utilities/try.h>

#include "ftcp.h"

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

struct ft_service {
	char *base_dir;
	cmn_rwfslock_t rwfslock;
	struct cmn_request_handler request_handler;
};

static int destroy_request_handler(cmn_request_handler_t handler) { return 0; }
static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket);
static char *get_file_list(ft_service_t ft_service);
static inline bool is_file_in_file_list(ft_service_t ft_service, char const *filename);
static inline int64_t get_file_length(FILE *file);
static void send_file_list(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_get_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_put_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);
static void handle_invalid_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request);

static struct cmn_request_handler_vtbl request_handler_ops_vtbl = {
	.destroy = destroy_request_handler,
	.handle_request = handle_request
};

extern ft_service_t ft_service_init(const char *base_dir) {
	ft_service_t ft_service;
#ifdef _DEBUG
	cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_ALL);
	cmn_logger_show_process_id(true);
	cmn_logger_show_thread_id(true);
	cmn_logger_show_source_file(true);
#endif // _DEBUG
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
	try(cmn_rwfslock_destroy(ft_service->rwfslock), -1, fail);
	free(ft_service->base_dir);
	free(ft_service);
	return 0;
fail:
	return 1;
}

extern cmn_request_handler_t ft_service_get_request_handler(ft_service_t ft_service) {
	return &(ft_service->request_handler);
}

static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket) {
	ft_service_t ft_service = container_of(handler, struct ft_service, request_handler);
	ftcp_preamble_packet_t request;
	try(cmn_socket2_recv(socket, request, FTCP_PREAMBLE_PACKET_SIZE), -1, fail);
	cmn_logger_log_debug("Received request");
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
	cmn_logger_log_debug("Replying to list request");
	char *file_list = get_file_list(ft_service);
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, strlen(file_list));
	try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail);
	try(cmn_socket2_ssend(socket, file_list), -1, fail);
	free(file_list);
fail:
	return;
}

static void handle_get_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	char required_filename[FTCP_PREAMBLE_PACKET_ARG_SIZE + 1] = { 0 };
	char *file_path;
	cmn_logger_log_debug("Replying to get request");
	memcpy(required_filename, ftcp_preamble_packet_arg(request), sizeof(uint8_t) * FTCP_PREAMBLE_PACKET_ARG_SIZE);
	if (required_filename[0] == '\0' || strstr(required_filename, "\n")) {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_INVALID_ARGUMENT, NULL, 0);
		cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
		return;
	}
	if (is_file_in_file_list(ft_service, required_filename)) {
		FILE *file;
		uint64_t file_length;
		try(asprintf(&file_path, "%s/%s", ft_service->base_dir, required_filename), NULL, fail);
		try(cmn_rwfslock_rdlock(ft_service->rwfslock, file_path), 1, fail);
		try(file = fopen(file_path, "r"), NULL, fail2);
		try(file_length = get_file_length(file), -1, fail2);
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_EXISTS, (void *)ftcp_preamble_packet_arg(request), file_length);
		try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail2);
		try(cmn_socket2_fsend(socket, file), -1, fail2);
		try(fclose(file), EOF, fail2);
		try(cmn_rwfslock_unlock(ft_service->rwfslock, file_path), 1, fail2);
		free(file_path);
	}
	else {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_NOT_EXISTS, NULL, 0);
		try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail);
	}
	return;
fail2:
	if (cmn_rwfslock_unlock(ft_service->rwfslock, file_path)) {
		cmn_logger_log_fatal("Can't unlock mutex on file!");
		exit(EXIT_FAILURE); // TODO: handle exit with a signal
	}
fail:
	cmn_logger_log_error("Something went wrong!");
}

static void handle_put_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	FILE *file;
	char *fpath;
	struct ftcp_result result;
	cmn_logger_log_debug("Replying to put request");
	try(asprintf(&fpath, "%s/%s", ft_service->base_dir, ftcp_preamble_packet_arg(request)), NULL, fail);
	result.value = access(fpath, F_OK) ? FTCP_RESULT_FILE_NOT_EXISTS_VALUE : FTCP_RESULT_FILE_EXISTS_VALUE;
	try(cmn_rwfslock_wrlock(ft_service->rwfslock, fpath), 1, fail);
	try(file = fopen(fpath, "w"), NULL, fail2);
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, result, NULL, 0);
	try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail2);
	try(cmn_socket2_frecv(socket, file, (long) ftcp_preamble_packet_data_packet_length(request)), -1, fail2);
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, 0);
	try(cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE), -1, fail2);
	try(fclose(file), -1, fail);
	try(cmn_rwfslock_unlock(ft_service->rwfslock, fpath), 1, fail2);
	free(fpath);
fail2:
	if (cmn_rwfslock_unlock(ft_service->rwfslock, fpath)) {
		cmn_logger_log_fatal("Can't unlock mutex on file!");
		exit(EXIT_FAILURE); // TODO: handle exit with a signal
	}
fail:
	cmn_logger_log_error("Something went wrong!");
}

static void handle_invalid_request(ft_service_t ft_service, cmn_socket2_t socket, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	cmn_logger_log_debug("Replying to invalid request");
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_ERROR, NULL, 0);
	cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE);
}

static char *get_file_list(ft_service_t ft_service) {
	char *list_command;
	try(asprintf(&list_command, "ls %s -p | grep -v /", ft_service->base_dir), -1, fail);
	FILE *pipe;
	char *filelist;
	pipe = popen(list_command, "r");
	fscanf(pipe, "%m[\x01-\xFF-]", &filelist);
	pclose(pipe);
	return filelist;
fail:
	cmn_logger_log_error("Something went wrong!");
}

static inline bool is_file_in_file_list(ft_service_t ft_service, char const *filename) {
	bool result;
	char *file_list;
	file_list = get_file_list(ft_service);
	result = strstr(file_list, filename) ? true : false;
	free(file_list);
	return result;
}

static inline int64_t get_file_length(FILE *file) {
	off_t result;
	off_t current_position;
	try(current_position = ftello(file), -1, fail);
	try(fseek(file, 0L, SEEK_END), -1, fail);
	try(result = ftello(file), -1, fail);
	try(fseek(file, 0L, current_position), -1, fail);
	return result;
fail:
	return -1;
}
