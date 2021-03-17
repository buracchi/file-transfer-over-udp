#include <stdio.h>
#include <stdlib.h>

#include "ftcp.h"
#include "socket2.h"
#include "try.h"
#include "utilities.h"

enum operation {
	OP_RUN,
	OP_SHOW_HELP,
	OP_FAIL
};

enum flag {
	F_INVALID,
	F_HELP,
	F_URL
};

struct option {
	enum flag flag;
	char* value;
};

static int usage();
static struct option** get_options(int argc, char* argv[]);

int ftc_start(char* url) {
	ftcp_message_t request = ftcp_message_init(COMMAND, LIST, NULL, 0, NULL);
	ftcp_message_t result;
	socket2_t socket;
	try(socket = socket2_init(TCP, IPV4), NULL);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", 1234), 1);
	try(socket2_connect(socket), 1);
	try(socket2_send(socket, request), -1);
	try(socket2_recv(socket, (char**)&result), -1);
	try(socket2_close(socket), 1);
	socket2_destroy(socket);
	printf("%s\n", ftcp_get_filename(result));
	free(request);
	free(result);
}

extern int main(int argc, char* argv[]) {
	enum operation operation = OP_RUN;
	int ret = EXIT_SUCCESS;
	char* url = NULL;
	struct option** options;
	try(options = get_options(argc, argv), NULL);
	for (int i = 0; options[i]; i++) {
		switch (options[i]->flag) {
		case F_URL:
			asprintf(&url, "%s", options[i]->value);
			break;
		case F_HELP:
			operation = OP_SHOW_HELP;
			goto body;
		default:
			operation = OP_FAIL;
			goto body;
		}
	}
	if (!url) {
		operation = OP_FAIL;
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
		try(ftc_start(url), 1);
		free(url);
	}
	return EXIT_SUCCESS;
}

static int usage() {
	try(fprintf(
			stderr, 
			"Usage: client <url>\n\
			\r\t-u, --url <url>\tURL to work with\n"
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
		else if (argc - (i + 1) > 0 && (streq(argv[i], "-u") || streq(argv[i], "--url"))) {
			options[last_index]->flag = F_URL;
			options[last_index]->value = argv[i + 1];
			i++;
		}
		else {
			options[last_index]->flag = F_URL;
			options[last_index]->value = argv[i + 1];
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
