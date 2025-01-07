#ifndef SESSION_OPTIONS_H
#define SESSION_OPTIONS_H

#include <stdbool.h>
#include <stddef.h>

#include <buracchi/tftp/server_session_stats.h>
#include <tftp.h>

struct session_options {
    const char **path;
    enum tftp_mode *mode;
    uint8_t *timeout;
    uint16_t *block_size;
    uint16_t *window_size;
    bool *adaptive_timeout;
    
    bool valid_options_required;
    bool options_acknowledged;
    struct tftp_option recognized_options[TFTP_OPTION_TOTAL_OPTIONS];
    char options_storage[tftp_request_packet_max_size - sizeof(enum tftp_opcode)];
    char *options_str;
    size_t options_str_size;
    const char *mode_str;
};

// if n > tftp_request_packet_max_size - sizeof(enum tftp_opcode) behaviour is undefined
bool session_options_init(struct session_options session_options[static 1],
                          size_t n,
                          const char options[static n],
                          const char *path[static 1],
                          enum tftp_mode mode[static 1],
                          uint8_t timeout[static 1],
                          uint16_t block_size[static 1],
                          uint16_t window_size[static 1],
                          bool adaptive_timeout[static 1],
                          struct tftp_session_stats_error error[static 1]);

bool parse_options(struct session_options options[static 1], int file_descriptor, bool is_adaptive_timeout_enabled);

enum tftp_read_type session_options_get_read_type(struct session_options options[static 1]);

#endif // SESSION_OPTIONS_H
