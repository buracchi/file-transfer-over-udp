#include "ft_service.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "try.h"
#include "utilities.h"

struct ft_service {
	char* base_dir;
};

extern ft_service_t ft_service_init(const char* base_dir) {
	ft_service_t service = malloc(sizeof *service);
	if (service) {
		size_t size = strlen(base_dir) + 1;
		char* tmp = malloc(sizeof *(service->base_dir) * size);
		memset(tmp, 0, size);
		strcpy(tmp, base_dir);
		service->base_dir = tmp;
	}
	return service;
}

extern int ft_service_destroy(const ft_service_t this) {
	free(this->base_dir);
	free(this);
	return 0;
}

extern const char* ft_service_get_base_dir(const ft_service_t this) {
	return this->base_dir;
}

extern char* ft_service_get_filelist(const ft_service_t this) {
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
