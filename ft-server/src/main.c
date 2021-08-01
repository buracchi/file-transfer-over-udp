#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "ft_service.h"
#include "utilities.h"
#include "try.h"
#include "fts.h"

#define DEFAULT_PORT 1234

enum operation {
	OP_RUN, 
	OP_SHOW_HELP, 
	OP_FAIL 
};

enum flag {
	F_INVALID,
	F_HELP,
	F_PORT,
	F_DIRECTORY
};

struct option {
	enum flag flag;
	char* value;
};

static int usage();
static struct option** get_options(int argc, char* argv[]);

extern int main(int argc, char* argv[]) {
	enum operation operation = OP_RUN;
	int port = DEFAULT_PORT;
	char directory[PATH_MAX] = {};
	struct option** options;
	ft_service_t ft_service;
	getcwd(directory, sizeof(directory));
	try(options = get_options(argc, argv), NULL);
	for (int i = 0; options[i]; i++) {
		switch (options[i]->flag) {
		case F_PORT:
			try(strtoi(options[i]->value, &port), 1);
			break;
		case F_DIRECTORY:
			strncpy(directory, options[i]->value, PATH_MAX - 1);
			break;
		case F_HELP:
			operation = OP_SHOW_HELP;
			goto body;
		default:
			operation = OP_FAIL;
			goto body;
		}
	}
body:
	for (int i = 0; options[i]; free(options[i++]));
	free(options);
	switch (operation) {
	case OP_FAIL:
		try(usage(), 1);
		return EXIT_FAILURE;
	case OP_SHOW_HELP:
		try(usage(), 1);
		return EXIT_SUCCESS;
	default:
		try(is_directory(directory), 0);
		try(ft_service = ft_service_init(directory), NULL);
		try(fts_start(port, ft_service), 1);
		ft_service_destroy(ft_service);
		free(ft_service);
		free(directory);
	}
	return EXIT_SUCCESS;
}

static int usage() {
	try(fprintf(
			stderr,
			"Usage: server [-p port] [-d directory]\n\
			\r\t-p, --port\n\
			\r\t\tspecify the port number (default port is " tostr(DEFAULT_PORT) ")\n\
			\r\t-d, --directory\n\
			\r\t\tspecify the shared directory (default directory is the current working directory)\n"
		) < 0,
		!0,
		fail
	);
	return 0;
fail:
	return 1;
}

static struct option** get_options(int argc, char* argv[]) {
	struct option** options;
	int last_index = 0;
	try(options = malloc(sizeof * options * argc), NULL, fail);
	memset(options, 0, sizeof * options * argc);
	for (int i = 1; i < argc; i++) {
		try(options[last_index] = malloc(sizeof(struct option)), NULL, fail_with_resources);
		if (streq(argv[i], "-h") || streq(argv[i], "--help")) {
			options[last_index]->flag = F_HELP;
		}
		else if (argc - (i + 1) > 0 && (streq(argv[i], "-p") || streq(argv[i], "--port"))) {
			options[last_index]->flag = F_PORT;
			options[last_index]->value = argv[i + 1];
			i++;
		}
		else if (argc - (i + 1) > 0 && (streq(argv[i], "-d") || streq(argv[i], "--directory"))) {
			options[last_index]->flag = F_DIRECTORY;
			options[last_index]->value = argv[i + 1];
			i++;
		}
		else {
			options[last_index]->flag = F_INVALID;
		}
		last_index++;
	}
	return options;
fail_with_resources:
	for (int i = 0; options[i]; free(options[i++]));
	free(options);
fail:
	return NULL;
}
