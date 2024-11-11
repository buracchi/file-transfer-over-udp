#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#ifdef __cplusplus

#include <cstdio>
#include <cstdint>

extern "C" {
#else
#include <stdio.h>
#include <stdint.h>
#endif

#include <logger.h>
#include <tftp.h>

enum command {
    CLIENT_COMMAND_LIST,
    CLIENT_COMMAND_GET,
    CLIENT_COMMAND_PUT,
};

union command_args {
    struct get_args {
        enum tftp_mode mode;            // transfer mode to use for the file transfer
        const char *filename;           // file to download from the server
        const char *output;             // file to save the downloaded file to
    } get;
    struct put_args {
        enum tftp_mode mode;            // transfer mode to use for the file transfer
        const char *filename;           // file to upload to the server
    } put;
};
struct options {
    uint8_t timeout_s;                      // duration of the timeout to use for the Go-Back N protocol, in seconds
    uint16_t block_size;                    // size of the data block to use for the file transfer
    uint16_t window_size;                   // size of the dispatch window to use for the Go-Back N protocol
    bool use_tsize;                         // flag to request the file size from the server
    bool adaptive_timeout;                  // flag to use an adaptive timeout calculated dynamically based on network delays
};

struct cli_args {
    const char *host;                       // IP address or hostname of the server to connect to
    const char *port;                       // port number to use for the connection
    enum command command;                   // command to send to the server (e.g. "list", "get", "put")
    union command_args command_args;
    struct options options;
    uint8_t retries;                        // number of retries to attempt before giving up
    enum logger_log_level verbose_level;    // verbose level to output additional information
};

bool cli_args_parse(struct cli_args *args, int argc, const char *argv[]);

void cli_args_destroy(struct cli_args *args);

#ifdef __cplusplus
}
#endif

#endif // CLI_ARGS_H
