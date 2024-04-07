#pragma once

#include <netdb.h>
#include <stdint.h>
#include <sys/socket.h>

#include <logger.h>
#include <tftp.h>

#include "tftp_client_stats.h"

#define UINT8_STRLEN 4
#define UINT16_STRLEN 6
#define SIZE_STRLEN 21

enum client_command {
    CLIENT_COMMAND_LIST,
    CLIENT_COMMAND_GET,
    CLIENT_COMMAND_PUT,
};

struct tftp_client {
    struct logger *logger;
    
    struct tftp_client_stats stats;
    void (*client_stats_callback)(struct tftp_client_stats stats[static 1]);
    
    int sockfd;
    struct sockaddr_storage server_addr_storage;
    socklen_t server_addr_len;
    struct sockaddr_storage sock_addr_storage;
    socklen_t sock_addr_len;
    char server_addr_str[INET6_ADDRSTRLEN];
    uint16_t server_port;
    char sock_addr_str[INET6_ADDRSTRLEN];
    uint16_t sock_port;
    
    enum client_command command;
    enum tftp_mode mode;
    const char *mode_str;
    const char *filename;
    uint8_t retries;
    
    struct tftp_rrq_packet rrq_packet;
    ssize_t rrq_packet_size;
    
    struct tftp_option options[TFTP_OPTION_TOTAL_OPTIONS];
    bool use_options;
    char options_str[tftp_option_formatted_string_max_size];
    bool server_maybe_do_not_support_options;   // if errors are received this flag could be set to true
    
    bool is_timeout_s_default;
    uint8_t timeout_s;
    char* timeout_s_str[UINT8_STRLEN];
    
    bool is_block_size_default;
    uint16_t block_size;
    char* block_size_str[UINT16_STRLEN];
    
    bool is_window_size_default;
    uint16_t window_size;
    char* window_size_str[UINT16_STRLEN];
    
    bool use_tsize;
    size_t tsize;
    char* tsize_str[SIZE_STRLEN];
    
    bool adaptive_timeout;
    double loss_probability;
};

struct tftp_client_arguments {
    const char *host;
    const char *port;
    enum client_command command;
    enum tftp_mode mode;
    const char *filename;
    uint8_t retries;
    uint8_t timeout_s;
    uint16_t block_size;
    uint16_t window_size;
    bool use_tsize;
    bool adaptive_timeout;
    double loss_probability;
    void (*client_stats_callback)(struct tftp_client_stats stats[static 1]);
};

bool tftp_client_init(struct tftp_client client[static 1], struct tftp_client_arguments args, struct logger logger[static 1]);

bool tftp_client_start(struct tftp_client client[static 1]);

void tftp_client_destroy(struct tftp_client client[static 1]);
