#include "ft_service.h"

#include <stdio.h>
#include <try.h>
#include <utilities.h>

extern char* get_filelist(const char* base_dir) {
	static const char* list_command;
	try(asprintf((char**)&list_command, "ls %s -p | grep -v /", base_dir), -1, fail);
	FILE* pipe;
	char* filelist;
	pipe = popen(list_command, "r");
	fscanf(pipe, "%m[\x01-\xFF-]", &filelist);
	pclose(pipe);
	return filelist;
fail:
	return NULL;
}

extern FILE load_file() {

}

extern void save_file() {

}
