#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "utilities.h"
#include "try.h"
#include "fts.h"

#define DEFAULT_PORT 1234

enum operation {
	RUN_SERVER, 
	SHOW_HELP, 
	FAIL 
};

enum flag {
	INVALID,
	HELP,
	PORT,
	DIRECTORY
};

struct option {
	enum flag flag;
	char* value;
};

static int usage();
static struct option** get_options(int argc, char* argv[]);

extern int main(int argc, char* argv[]) {
	enum operation operation = RUN_SERVER;
	int ret = EXIT_SUCCESS;
	int port = DEFAULT_PORT;
	char directory[PATH_MAX] = {};
	struct option** options;
	getcwd(directory, sizeof(directory));
	try(options = get_options(argc, argv), NULL);
	for (int i = 0; options[i]; i++) {
		switch (options[i]->flag) {
		case PORT:
			try(strtoi(options[i]->value, &port), 1);
			break;
		case DIRECTORY:
			strncpy(directory, options[i]->value, PATH_MAX - 1);
			break;
		case HELP:
			operation = SHOW_HELP;
			goto body;
		default:
			operation = FAIL;
			goto body;
		}
	}
body:
	for (int i = 0; options[i]; free(options[i++]));
	free(options);
	switch (operation) {
	case FAIL:
		try(usage(), 1);
		return EXIT_FAILURE;
	case SHOW_HELP:
		try(usage(), 1);
		return EXIT_SUCCESS;
	default:
		try(is_directory(directory), 0);
		try(fts_start(port, directory), 1);
	}
	return EXIT_SUCCESS;
}

static int usage() {
	try(
		fprintf(
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
			options[last_index]->flag = HELP;
		}
		else if (argc - (i + 1) > 0 && (streq(argv[i], "-p") || streq(argv[i], "--port"))) {
			options[last_index]->flag = PORT;
			options[last_index]->value = argv[i + 1];
			i++;
		}
		else if (argc - (i + 1) > 0 && (streq(argv[i], "-d") || streq(argv[i], "--directory"))) {
			options[last_index]->flag = DIRECTORY;
			options[last_index]->value = argv[i + 1];
			i++;
		}
		else {
			options[last_index]->flag = INVALID;
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