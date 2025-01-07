#ifndef CMDLINE_ARGS_H
#define CMDLINE_ARGS_H

#ifdef __cplusplus
#include <cstdio>
#include <cstdint>

extern "C" {
#else
#include <stdio.h>
#include <stdint.h>
#endif

#include <logger.h>

struct cli_args {
    const char *host;                       // host to listen on
    const char *port;                       // port to listen on
    const char *root;                       // file root directory
    uint16_t workers;                       // number of worker threads to use
    uint16_t max_worker_sessions;           // maximum number of sessions a worker can handle
    uint8_t retries;                        // number of retries to attempt before giving up
    uint8_t timeout_s;                      // duration of the timeout in seconds
    double loss_probability;                // probability of packet loss to simulate
    enum logger_log_level verbose_level;    // verbose level to output additional information
    bool enable_write_requests;             // flag to enable write requests
    bool enable_list_requests;              // flag to enable list requests
    bool enable_adaptive_timeout;           // flag to enable adaptive timeout requests calculated dynamically based on network delays
};

bool cli_args_parse(struct cli_args* args, int argc, const char *argv[]);

void cli_args_destroy(struct cli_args *args);

#ifdef __cplusplus
}
#endif

#endif // CMDLINE_ARGS_H
