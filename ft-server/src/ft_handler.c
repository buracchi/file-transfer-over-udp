#include "ft_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __unix__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#include "ft_service.h"
#include "ftcp.h"
#include "socket2.h"
#include "rwfslock.h"
#include "try.h"
#include "utilities.h"

typedef struct ft_handler {
	const ft_service_t ft_service;
} *_ft_handler_t;

static void handle_list_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request);
static void handle_get_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request);
static void handle_put_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request);
static void handle_invalid_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request);

static struct rwfslock rwfslock;

extern ft_handler_t ft_handler_init(const ft_service_t ft_service) {
	_ft_handler_t handler = malloc(sizeof * handler);
	if (handler) {
		*((void**)&(handler->ft_service)) = *((void**)&ft_service);
		rwfslock_init(&rwfslock);
	}
	return handler;
}

extern int ft_handler_destroy(const ft_handler_t ft_handler) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	rwfslock_destroy(&rwfslock);
	return 0;
}

extern void ft_handler_handle_request(const ft_handler_t ft_handler, struct socket2* socket) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	ftcp_pp_t request = malloc(ftcp_pp_size());
	socket2_recv(socket, request, ftcp_pp_size());
	switch (ftcp_get_type(request)) {
	case COMMAND:
		switch (ftcp_get_operation(request)) {
		case LIST:
			handle_list_request(this, socket, request);
			break;
		case GET:
			handle_get_request(this, socket, request);
			break;
		case PUT:
			handle_put_request(this, socket, request);
			break;
		default:
			handle_invalid_request(this, socket, request);
		}
		break;
	default:
		handle_invalid_request(this, socket, request);
	}
	delete(socket);
	free(request);
}

static void handle_list_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	ftcp_pp_t reply;
	FILE* pipe;
	char* filelist = ft_service_get_filelist(this->ft_service);
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, strlen(filelist));
	socket2_send(socket, reply, ftcp_pp_size());
	socket2_ssend(socket, filelist);
	free(filelist);
	free(reply);
}

static void handle_get_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	ftcp_pp_t reply;
	char* frpath;
	char* fpath;
	FILE* file;
	uint64_t flen;
	bool fexist;
	frpath = ftcp_get_arg(request);
	if (strstr(frpath, "/")) {
		reply = ftcp_pp_init(RESPONSE, INVALID_ARGUMENT, NULL, 0);
		socket2_send(socket, reply, ftcp_pp_size());
		return;
	}
	asprintf(&fpath, "%s/%s", ft_service_get_base_dir(this->ft_service), frpath);
	if (!access(fpath, F_OK)) {
		try(rwfslock_rdlock(&rwfslock, fpath), 1, fail);
		file = fopen(fpath, "r");
		fseek(file, 0L, SEEK_END);
		flen = ftell(file);
		fseek(file, 0L, SEEK_SET);
		reply = ftcp_pp_init(RESPONSE, FILE_EXIST, frpath, flen);
		socket2_send(socket, reply, ftcp_pp_size());
		socket2_fsend(socket, file);
		fclose(file);
		try(rwfslock_unlock(&rwfslock, fpath), 1, fail);
	}
	else {
		reply = ftcp_pp_init(RESPONSE, FILE_NOT_EXIST, NULL, 0);
		socket2_send(socket, reply, ftcp_pp_size());
	}
	free(fpath);
	free(reply);
fail:
	return;
}

static void handle_put_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	ftcp_pp_t reply;
	FILE* file;
	char* fpath;
	enum ftcp_result result;
	asprintf(&fpath, "%s/%s", ft_service_get_base_dir(this->ft_service), ftcp_get_arg(request));
	result = access(fpath, F_OK) ? FILE_NOT_EXIST : FILE_EXIST;
	try(rwfslock_wrlock(&rwfslock, fpath), 1, fail);
	file = fopen(fpath, "w");
	reply = ftcp_pp_init(RESPONSE, result, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
	socket2_frecv(socket, file, ftcp_get_dplen(request));
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	fclose(file);
	try(rwfslock_unlock(&rwfslock, fpath), 1, fail);
	free(fpath);
	free(reply);
fail:
	return;
}

static void handle_invalid_request(const ft_handler_t ft_handler, struct socket2* socket, ftcp_pp_t request) {
	_ft_handler_t this = (_ft_handler_t)ft_handler;
	ftcp_pp_t reply;
	reply = ftcp_pp_init(RESPONSE, ERROR, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
}
