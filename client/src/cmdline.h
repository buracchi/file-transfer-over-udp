#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct cmdline_args {
	char *host;              // IP address or hostname of the server to connect to
	char *port;              // port number to use for the connection
	char const *command;     // command to send to the server (e.g. "list", "get", "put")
	char *filename;          // file to send or receive (used with the "get" and "put" commands)
	uint32_t window_size;    // size of the dispatch window to use for the Go-Back N protocol
	long timeout_duration;   // duration of the timeout to use for the Go-Back N protocol, in milliseconds
	bool adaptive_timeout;   // flag to use an adaptive timeout calculated dynamically based on network delays
	double loss_probability; // probability of packet loss to simulate
	int verbose_level;       // verbose level to output additional information
} cmdline_args_t;

/**
 * Parses command-line arguments and fills in the fields of a `cmdline_args` struct.
 * @param argc The number of command-line arguments passed to the program.
 * @param argv An array of `char*` representing the command-line arguments passed to the program.
 * @param args A pointer to a `cmdline_args` struct to store the parsed arguments.
 * @return 0 on success, 1 on failure.
 */
int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t *args);
