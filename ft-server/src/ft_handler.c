#include "ft_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __unix__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#include "types/request_handler.h"
#include "ft_service.h"
#include "ftcp.h"
#include "socket2.h"
#include "rwfslock.h"
#include "try.h"
#include "utilities.h"

struct ft_handler {
	struct cmn_request_handler super;
	ft_service_t ft_service;
	cmn_rwfslock_t rwfslock;
};

static struct cmn_request_handler_vtbl* get_request_handler_vtbl();
static int destroy(cmn_request_handler_t handler);
static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket);
static void handle_list_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request);
static void handle_get_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request);
static void handle_put_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request);
static void handle_invalid_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request);

extern ft_handler_t ft_handler_init(const char* base_dir_path) {
	ft_handler_t this = malloc(sizeof * this);
	if (this) {
		this->super.__ops_vptr = get_request_handler_vtbl();
		this->ft_service = ft_service_init(base_dir_path);
		this->rwfslock = cmn_rwfslock_init();
	}
	return this;
}

static struct cmn_request_handler_vtbl* get_request_handler_vtbl() {
	struct cmn_request_handler_vtbl vtbl_zero = { 0, 0 };
	if (!memcmp(&vtbl_zero, &__cmn_request_handler_ops_vtbl, sizeof * &__cmn_request_handler_ops_vtbl)) {
		__cmn_request_handler_ops_vtbl.destroy = destroy;
		__cmn_request_handler_ops_vtbl.handle_request = handle_request;
	}
	return &__cmn_request_handler_ops_vtbl;
}

static int destroy(cmn_request_handler_t handler) {
	ft_handler_t this = (ft_handler_t)handler;
	ft_service_destroy(this->ft_service);
	cmn_rwfslock_destroy(this->rwfslock);
	return 0;
}

static void handle_request(cmn_request_handler_t handler, cmn_socket2_t socket) {
	ft_handler_t this = (ft_handler_t)handler;
	ftcp_pp_t request;
	request = malloc(ftcp_pp_size());
	cmn_socket2_recv(socket, request, ftcp_pp_size());
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
	free(request);
}

static void handle_list_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	char* filelist = ft_service_get_filelist(this->ft_service);
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, strlen(filelist));
	cmn_socket2_send(socket, reply, ftcp_pp_size());
	cmn_socket2_ssend(socket, filelist);
	free(filelist);
	free(reply);
}

static void handle_get_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	char* frpath;
	char* fpath;
	FILE* file;
	uint64_t flen;
	frpath = ftcp_get_arg(request);
	if (strstr(frpath, "/")) {
		reply = ftcp_pp_init(RESPONSE, INVALID_ARGUMENT, NULL, 0);
		cmn_socket2_send(socket, reply, ftcp_pp_size());
		return;
	}
	asprintf(&fpath, "%s/%s", ft_service_get_base_dir(this->ft_service), frpath);
	if (!access(fpath, F_OK)) {
		try(cmn_rwfslock_rdlock(this->rwfslock, fpath), 1, fail);
		file = fopen(fpath, "r");
		fseek(file, 0L, SEEK_END);
		flen = ftell(file);
		fseek(file, 0L, SEEK_SET);
		reply = ftcp_pp_init(RESPONSE, FILE_EXIST, frpath, flen);
		cmn_socket2_send(socket, reply, ftcp_pp_size());
		cmn_socket2_fsend(socket, file);
		fclose(file);
		try(cmn_rwfslock_unlock(this->rwfslock, fpath), 1, fail);
	}
	else {
		reply = ftcp_pp_init(RESPONSE, FILE_NOT_EXIST, NULL, 0);
		cmn_socket2_send(socket, reply, ftcp_pp_size());
	}
	free(fpath);
	free(reply);
fail:
	return;
}

static void handle_put_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	FILE* file;
	char* fpath;
	enum ftcp_result result;
	asprintf(&fpath, "%s/%s", ft_service_get_base_dir(this->ft_service), ftcp_get_arg(request));
	result = access(fpath, F_OK) ? FILE_NOT_EXIST : FILE_EXIST;
	try(cmn_rwfslock_wrlock(this->rwfslock, fpath), 1, fail);
	file = fopen(fpath, "w");
	reply = ftcp_pp_init(RESPONSE, result, NULL, 0);
	cmn_socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
	cmn_socket2_frecv(socket, file, ftcp_get_dplen(request));
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, 0);
	cmn_socket2_send(socket, reply, ftcp_pp_size());
	fclose(file);
	try(cmn_rwfslock_unlock(this->rwfslock, fpath), 1, fail);
	free(fpath);
	free(reply);
fail:
	return;
}

static void handle_invalid_request(ft_handler_t this, cmn_socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	reply = ftcp_pp_init(RESPONSE, ERROR, NULL, 0);
	cmn_socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
}
