#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct cmdline_args {
	char *listen_port;         // port number to listen on
	char *directory;           // file root directory
	unsigned int thread_count; // number of worker threads to use
	uint32_t window_size;      // size of the dispatch window to use for the Go-Back N protocol
	long timeout_duration;     // duration of the timeout to use for the Go-Back N protocol, in milliseconds
	bool adaptive_timeout;     // flag to use an adaptive timeout calculated dynamically based on network delays
	double loss_probability;   // probability of packet loss to simulate
	int verbose_level;         // verbose level to output additional information
} cmdline_args_t;

/**
 * @brief Parse the command line arguments.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @param args The structure to fill with the parsed arguments.
 * @return 0 on success, 1 on failure
 */
int parse_cmdline_args(int argc, const char *argv[static const argc + 1], cmdline_args_t args[static 1]);
