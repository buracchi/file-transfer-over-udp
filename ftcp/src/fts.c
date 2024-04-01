#include <fts/fts.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
#include <unistd.h>
#endif

#include <buracchi/common/concurrency/rwfslock.h>
#include <buracchi/common/logger/logger.h>
#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/try.h>
#include <buracchi/common/utilities/utilities.h>

#include "ftcp.h"
#include "client.h"
#include "session.h"

#define INITIAL_FILE_LIST_SIZE 512

struct fts {
	cmn_rwfslock_t rwfslock;
};

static fts_error_t handle_list_request(fts_t fts, cmn_socket2_t socket, char const *directory);
static fts_error_t handle_get_request(fts_t fts, cmn_socket2_t socket, char const *directory, ftcp_preamble_packet_t request);
static fts_error_t handle_put_request(fts_t fts, cmn_socket2_t socket, char const *directory, ftcp_preamble_packet_t request);
static fts_error_t handle_invalid_request(fts_t fts, cmn_socket2_t socket);
static char *get_file_list(char const *directory);
static bool is_file_in_file_list(char const *directory, char const *filename);
static int get_file_length(FILE *file, uint64_t *length);

extern fts_t fts_init(void) {
#ifdef _DEBUG
	cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_ALL);
	cmn_logger_show_process_id(true);
	cmn_logger_show_thread_id(true);
	cmn_logger_show_source_file(true);
#endif // _DEBUG
	fts_t fts;
	try(fts = malloc(sizeof *fts), NULL, fail);
	try(fts->rwfslock = cmn_rwfslock_init(), NULL, fail2);
	return fts;
fail2:
	free(fts);
fail:
	return NULL;
}

extern int fts_destroy(fts_t fts) {
	// TODO: close all sessions
	try(cmn_rwfslock_destroy(fts->rwfslock), -1, fail);
	free(fts);
	return 0;
fail:
	return 1;
}

extern struct fts_clinet_connection_t fts_create_client_connection(fts_t fts, char const *url) {
}

extern char const *fts_error_to_str(fts_error_t err) {
	switch (err) {
	case FTS_ERROR_SUCCESS:
		return "FTS_ERROR_SUCCESS";
	case FTS_ERROR_NETWORK:
		return "FTS_ERROR_NETWORK";
	case FTS_ERROR_FILE_ALREADY_EXISTS:
		return "FTS_ERROR_FILE_ALREADY_EXISTS";
	case FTS_ERROR_FILE_NOT_EXISTS:
		return "FTS_ERROR_FILE_NOT_EXISTS";
	case FTS_ERROR_IO_ERROR:
		return "FTS_ERROR_IO_ERROR";
	case FTS_ERROR_ENOMEM:
		return "FTS_ERROR_ENOMEM";
	case FTS_ERROR_LOCK_ERROR:
		return "FTS_ERROR_LOCK_ERROR";
	default:
		break;
	}
	return "FTS_ERROR_UNKNOWN_ERROR_CODE";
}

extern fts_error_t fts_get_file_list(fts_session_t session, char **const file_list) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	char *buffer;
	if (session->state.last_operation_waiting) {
		fts_reset_session(session); // TODO: handle error
	}
	ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_LIST, NULL, 0);
	if (cmn_socket2_send(session->sock, request, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
		return FTS_ERROR_NETWORK;
	}
	if (cmn_socket2_recv(session->sock, response, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
		return FTS_ERROR_NETWORK;
	}
	if (cmn_socket2_srecv(session->sock, &buffer) == -1) {
		return FTS_ERROR_NETWORK;
	}
	*file_list = buffer;
	return FTS_ERROR_SUCCESS;
}

extern fts_error_t fts_download_file(fts_session_t session, char const *file_name, enum fts_option option) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	FILE *file;
	struct ftcp_arg filename_buff = { 0 };
	if (session->state.last_operation_waiting && (session->state.last_operation != GET || session->state.last_operation_file_name_arg != file_name)) {
		fts_reset_session(session); // TODO: handle error
	}
	if (!session->state.last_operation_waiting) {
		session->state.last_operation = GET;
		session->state.last_operation_file_name_arg = file_name;
		strncpy((char *)&filename_buff, file_name, sizeof(filename_buff));
		ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_GET, &filename_buff, 0);
		if (cmn_socket2_send(session->sock, request, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (cmn_socket2_recv(session->sock, response, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (ftcp_preamble_packet_result(response) != FTCP_RESULT_FILE_EXISTS_VALUE) {
			return FTS_ERROR_FILE_NOT_EXISTS;
		}
		session->state.last_operation_file_len_arg = ftcp_preamble_packet_data_packet_length(response);
		session->state.last_operation_waiting = true;
	}
	if (option == DO_NOT_REPLACE && !access(file_name, F_OK)) {
		return FTS_ERROR_FILE_ALREADY_EXISTS;
	}
	if ((file = fopen(file_name, "w")) == NULL) {
		return FTS_ERROR_IO_ERROR;
	}
	if (cmn_rwfslock_wrlock(session->fts->rwfslock, file_name) == 1) {
		return FTS_ERROR_LOCK_ERROR;
	}
	if (cmn_socket2_frecv(session->sock, file, (long)session->state.last_operation_file_len_arg) == -1) {
		return FTS_ERROR_NETWORK;
	}
	if (cmn_rwfslock_unlock(session->fts->rwfslock, file_name) == 1) {
		return FTS_ERROR_LOCK_ERROR;
	}
	if (fclose(file)) {
		return FTS_ERROR_IO_ERROR;
	}
	session->state.last_operation_waiting = false;
	return FTS_ERROR_SUCCESS;
}

extern fts_error_t fts_upload_file(fts_session_t session, char const *file_name, enum fts_option option) {
	ftcp_preamble_packet_t request;
	ftcp_preamble_packet_t response;
	struct ftcp_arg filename_buff = { 0 };
	FILE *file;
	uint64_t file_length;
	if (session->state.last_operation_waiting && (session->state.last_operation != PUT || session->state.last_operation_file_name_arg != file_name)) {
		fts_reset_session(session); // TODO: handle error
	}
	if (!session->state.last_operation_waiting) {
		session->state.last_operation = PUT;
		session->state.last_operation_file_name_arg = file_name;
		if ((file = fopen(file_name, "r")) == NULL) {
			if (errno == ENOENT) {
				return FTS_ERROR_FILE_NOT_EXISTS;
			}
			return FTS_ERROR_IO_ERROR;
		}
		strncpy((char *)&filename_buff, file_name, sizeof(filename_buff));
		if (cmn_rwfslock_rdlock(session->fts->rwfslock, file_name) == 1) {
			return FTS_ERROR_LOCK_ERROR;
		}
		if (get_file_length(file, &file_length) == INT64_C(-1)) {
			return FTS_ERROR_IO_ERROR;
		}
		ftcp_preamble_packet_init(&request, FTCP_TYPE_COMMAND, FTCP_OPERATION_PUT, &filename_buff, file_length);
		if (cmn_socket2_send(session->sock, request, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (cmn_socket2_recv(session->sock, response, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		session->state.last_operation_file_len_arg = file_length;
		session->state.last_operation_waiting = true;
	}
	if (option == DO_NOT_REPLACE && ftcp_preamble_packet_result(response) == FTCP_RESULT_FILE_EXISTS_VALUE) {
		return FTS_ERROR_FILE_ALREADY_EXISTS;
	}
	if (cmn_socket2_fsend(session->sock, file) == -1) {
		return FTS_ERROR_NETWORK;
	}
	if (cmn_rwfslock_unlock(session->fts->rwfslock, file_name) == 1) {
		return FTS_ERROR_LOCK_ERROR;
	}
	if (fclose(file)) {
		return FTS_ERROR_IO_ERROR;
	}
	if (cmn_socket2_recv(session->sock, response, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
		return FTS_ERROR_NETWORK;
	}
	// TODO: handle result
	session->state.last_operation_waiting = false;
	return FTS_ERROR_SUCCESS;
}

extern fts_error_t fts_handle_request(fts_t fts, cmn_socket2_t socket, char const *directory) {
	ftcp_preamble_packet_t request;
	if (cmn_socket2_recv(socket, request, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
		return FTS_ERROR_NETWORK;
	}
	cmn_logger_log_debug("Received request");
	if (ftcp_preamble_packet_type(request) == FTCP_TYPE_COMMAND_VALUE) {
		switch (ftcp_preamble_packet_operation(request)) {
		case FTCP_OPERATION_LIST_VALUE:
			return handle_list_request(fts, socket, directory);
		case FTCP_OPERATION_GET_VALUE:
			return handle_get_request(fts, socket, directory, request);
		case FTCP_OPERATION_PUT_VALUE:
			return handle_put_request(fts, socket, directory, request);
		default:
			break;
		}
	}
	return handle_invalid_request(fts, socket);
}

static fts_error_t handle_list_request(fts_t fts, cmn_socket2_t socket, char const *directory) {
	ftcp_preamble_packet_t reply;
	char *file_list;
	cmn_logger_log_debug("Replying to list request");
	if ((file_list = get_file_list(directory)) == NULL) {
		return FTS_ERROR_IO_ERROR; // TODO: check if this is the right
		                           // error value
	}
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, strlen(file_list));
	if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
		return FTS_ERROR_NETWORK;
	}
	if (cmn_socket2_ssend(socket, file_list) == -1) {
		return FTS_ERROR_NETWORK;
	}
	free(file_list);
	return FTS_ERROR_SUCCESS;
	(void)fts;
}

static fts_error_t handle_get_request(fts_t fts, cmn_socket2_t socket, char const *directory, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	ftcp_arg_t request_arg = ftcp_preamble_packet_arg(request);
	char required_filename[FTCP_PREAMBLE_PACKET_ARG_SIZE + 1] = { 0 };
	cmn_logger_log_debug("Replying to get request");
	memcpy(required_filename, request_arg, sizeof *request_arg);
	if (required_filename[0] == '\0' || strstr(required_filename, "\n")) {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_INVALID_ARGUMENT, NULL, 0);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
	}
	else if (is_file_in_file_list(directory, required_filename)) {
		// TODO: unlock on errors
		char *file_path;
		FILE *file;
		uint64_t file_length;
		if (asprintf(&file_path, "%s/%s", directory, required_filename) == -1) {
			return FTS_ERROR_ENOMEM;
		}
		if (cmn_rwfslock_rdlock(fts->rwfslock, file_path) == 1) {
			return FTS_ERROR_LOCK_ERROR;
		}
		if ((file = fopen(file_path, "r")) == NULL) {
			return FTS_ERROR_IO_ERROR;
		}
		if (get_file_length(file, &file_length) == -1) {
			return FTS_ERROR_IO_ERROR;
		}
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_EXISTS, request_arg, file_length);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE)) {
			return FTS_ERROR_NETWORK;
		}
		if (cmn_socket2_fsend(socket, file) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (fclose(file) == EOF) {
			return FTS_ERROR_IO_ERROR;
		}
		if (cmn_rwfslock_unlock(fts->rwfslock, file_path) == 1) {
			return FTS_ERROR_LOCK_ERROR;
		}
		free(file_path);
	}
	else {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_NOT_EXISTS, NULL, 0);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
	}
	return FTS_ERROR_SUCCESS;
}

static fts_error_t handle_put_request(fts_t fts, cmn_socket2_t socket, char const *directory, ftcp_preamble_packet_t request) {
	ftcp_preamble_packet_t reply;
	ftcp_arg_t request_arg = ftcp_preamble_packet_arg(request);
	struct ftcp_result result;
	char filename[FTCP_PREAMBLE_PACKET_ARG_SIZE + 1] = { 0 };
	cmn_logger_log_debug("Replying to put request");
	memcpy(filename, request_arg, sizeof *request_arg);
	if (filename[0] == '\0') {
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_INVALID_ARGUMENT, NULL, 0);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
	}
	else {
		// TODO: unlock on errors
		char *file_path;
		FILE *file;
		if (asprintf(&file_path, "%s/%s", directory, filename) == -1) {
			return FTS_ERROR_ENOMEM;
		}
		result.value = access(file_path, F_OK) ? FTCP_RESULT_FILE_NOT_EXISTS_VALUE : FTCP_RESULT_FILE_EXISTS_VALUE;
		if (cmn_rwfslock_rdlock(fts->rwfslock, file_path) == 1) {
			return FTS_ERROR_LOCK_ERROR;
		}
		if ((file = fopen(file_path, "w")) == NULL) {
			return FTS_ERROR_IO_ERROR;
		}
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, result, NULL, 0);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (cmn_socket2_frecv(socket, file, (long)ftcp_preamble_packet_data_packet_length(request)) == -1) {
			return FTS_ERROR_NETWORK;
		}
		ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, 0);
		if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE) == -1) {
			return FTS_ERROR_NETWORK;
		}
		if (fclose(file) == EOF) {
			return FTS_ERROR_IO_ERROR;
		}
		if (cmn_rwfslock_unlock(fts->rwfslock, file_path) == 1) {
			return FTS_ERROR_LOCK_ERROR;
		}
		free(file_path);
	}
	return FTS_ERROR_SUCCESS;
}

static fts_error_t handle_invalid_request(fts_t fts, cmn_socket2_t socket) {
	ftcp_preamble_packet_t reply;
	cmn_logger_log_debug("Replying to invalid request");
	ftcp_preamble_packet_init(&reply, FTCP_TYPE_RESPONSE, FTCP_RESULT_ERROR, NULL, 0);
	if (cmn_socket2_send(socket, reply, FTCP_PREAMBLE_PACKET_SIZE)) {
		return FTS_ERROR_NETWORK;
	}
	return FTS_ERROR_SUCCESS;
	(void)fts;
}

static inline int get_file_length(FILE *file, uint64_t *const length) {
	off_t current_position;
	off_t result;
	try(current_position = ftello(file), -1, fail);
	try(fseek(file, 0L, SEEK_END), -1, fail);
	try(result = ftello(file), -1, fail);
	try(fseek(file, 0L, current_position), -1, fail);
	*length = result;
	return 0;
fail:
	return -1;
}

static char *get_file_list(char const *directory) {
	char *file_list;
	size_t file_list_size;
	char *list_command;
	FILE *pipe;
	int input;
	size_t input_length = 0;
	file_list_size = INITIAL_FILE_LIST_SIZE;
	try(file_list = malloc(file_list_size), NULL, fail);
	try(asprintf(&list_command, "ls %s -p | grep -v /", directory), -1, fail2);
	try(pipe = popen(list_command, "r"), NULL, fail3);
	do {
		input = getc(pipe);
		if (input == EOF) {
			if (feof(pipe)) {
				input = '\0';
			}
			else {
				int error;
				error = ferror(pipe);
				cmn_logger_log_error("Error %s: %s", error, strerror(error));
				goto fail;
			}
		}
		input_length++;
		if (input_length > file_list_size) {
			char *tmp;
			file_list_size <<= 1;
			try(tmp = realloc(file_list, file_list_size), NULL, fail4);
			file_list = tmp;
		}
		file_list[input_length - 1] = (char)input;
	}
	while (input != '\0');
	try(pclose(pipe), -1, fail);
	return file_list;
fail4:
	pclose(pipe);
fail3:
	free(list_command);
fail2:
	free(file_list);
fail:
	cmn_logger_log_error("Something went wrong!");
	return NULL;
}

static inline bool is_file_in_file_list(char const *directory, char const *filename) {
	bool result;
	char *file_list;
	file_list = get_file_list(directory);
	result = strstr(file_list, filename) ? true : false;
	free(file_list);
	return result;
}
