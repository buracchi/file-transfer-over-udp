#include "ft_service.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "try.h"
#include "utilities.h"

typedef struct ft_service {
	const char* base_dir;
} *_ft_service_t;

extern ft_service_t ft_service_init(const char* base_dir) {
	_ft_service_t service = malloc(sizeof *service);
	if (service) {
		size_t size = strlen(base_dir) + 1;
		char* tmp = malloc(sizeof *(service->base_dir) * size);
		memset(tmp, 0, size);
		strcpy(tmp, base_dir);
		service->base_dir = (const char*)tmp;
	}
	return service;
}

extern int ft_service_destroy(const ft_service_t ft_service) {
	_ft_service_t this = (_ft_service_t)ft_service;
	free((void*)this->base_dir);
	return 0;
}

extern const char* ft_service_get_base_dir(const ft_service_t ft_service) {
	_ft_service_t this = (_ft_service_t)ft_service;
	return this->base_dir;
}

extern char* ft_service_get_filelist(const ft_service_t ft_service) {
	_ft_service_t this = (_ft_service_t)ft_service;
	static const char* list_command;
	try(asprintf((char**)&list_command, "ls %s -p | grep -v /", this->base_dir), -1, fail);
	FILE* pipe;
	char* filelist;
	pipe = popen(list_command, "r");
	fscanf(pipe, "%m[\x01-\xFF-]", &filelist);
	pclose(pipe);
	return filelist;
fail:
	return NULL;
}

extern FILE ft_service_load_file(const ft_service_t ft_service) {
	_ft_service_t this = (_ft_service_t)ft_service;

}

extern void ft_service_save_file(const ft_service_t ft_service) {
	_ft_service_t this = (_ft_service_t)ft_service;

}
