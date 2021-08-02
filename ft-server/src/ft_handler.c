#include "ft_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __unix__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#include "struct_ft_handler.h"
#include "ft_service.h"
#include "ftcp.h"
#include "socket2.h"
#include "rwfslock.h"
#include "try.h"
#include "utilities.h"

static struct ft_handler_vtbl* get_ft_handler_vtbl();
static int _destroy(struct ft_handler* this);
static void handle_request(struct request_handler* pthis, struct socket2* socket);
static void handle_list_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request);
static void handle_get_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request);
static void handle_put_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request);
static void handle_invalid_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request);

struct rwfslock rwfslock;

extern ft_handler_t ft_handler_init(const ft_service_t ft_service) {
	struct ft_handler* ft_handler = malloc(sizeof * ft_handler);
	if (ft_handler) {
		_ft_handler_init(ft_handler, ft_service);
	}
	return ft_handler;
}

extern int ft_handler_destroy(ft_handler_t ft_handler) {
	struct ft_handler* this = (struct ft_handler*)ft_handler;
	return _ft_handler_ops(this)->destroy(this);
}

extern void _ft_handler_init(struct ft_handler* this, const ft_service_t ft_service) {
	_request_handler_init((void*)this);
	this->__ops_vptr = get_ft_handler_vtbl();
	//_ft_handler_ops(this) = get_ft_handler_vtbl();
	*((ft_service_t*)&(this->ft_service)) = ft_service;
	rwfslock_init(&rwfslock);
}

static struct ft_handler_vtbl* get_ft_handler_vtbl() {
	struct ft_handler_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__ft_handler_ops_vtbl, sizeof * &__ft_handler_ops_vtbl)) {
		memcpy(&__ft_handler_ops_vtbl, &__request_handler_ops_vtbl, sizeof(struct ft_handler_vtbl) - sizeof(struct request_handler_vtbl));
		__ft_handler_ops_vtbl.destroy = _destroy;
		__ft_handler_ops_vtbl.handle_request = handle_request;	// override
	}
	return &__ft_handler_ops_vtbl;
}

static int _destroy(struct ft_handler* this) {
	return rwfslock_destroy(&rwfslock);
}

static void handle_request(struct request_handler* pthis, struct socket2* socket) {
	struct ft_handler* this = (struct ft_handler*)pthis;
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

static void handle_list_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	char* filelist = ft_service_get_filelist(this->ft_service);
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, strlen(filelist));
	socket2_send(socket, reply, ftcp_pp_size());
	socket2_ssend(socket, filelist);
	free(filelist);
	free(reply);
}

static void handle_get_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	char* frpath;
	char* fpath;
	FILE* file;
	uint64_t flen;
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

static void handle_put_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request) {
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

static void handle_invalid_request(struct ft_handler* this, struct socket2* socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	reply = ftcp_pp_init(RESPONSE, ERROR, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
}
