#include <buracchi/tftp/client.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include <tftp.h>

#include "tftp_client_connection.h"
#include "../utils/inet.h"

#define UINT8_STRLEN 4
#define UINT16_STRLEN 6
#define SIZE_STRLEN 21

enum request_type {
    REQUEST_LIST,
    REQUEST_GET,
    REQUEST_PUT,
};

struct get_request {
    const char *filename;
    enum tftp_mode mode;
    const char *mode_str;
    const char *output_path;
};

struct put_request {
    const char *filename;
    enum tftp_mode mode;
    const char *mode_str;
};

struct options {
    struct tftp_option options[TFTP_OPTION_TOTAL_OPTIONS];
    char options_str[tftp_option_formatted_string_max_size];
    
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
    
    bool use_adaptive_timeout;
};

struct request {
    struct logger *logger;
    struct tftp_client_connection *connection;
    struct tftp_client_stats stats;
    void (*client_stats_callback)(struct tftp_client_stats stats[static 1]);
    uint8_t retries;
    bool use_options;
    struct options options;
    
    enum request_type request_type;
    union {
        struct get_request get;
        struct put_request put;
    } details;
    
    union {
        struct tftp_rrq_packet rrq_packet;
        struct tftp_ack_packet ack_packet;
        // error packet are not acknowledged or retransmitted, so they don't need to be stored
    } last_packet_sent;
    size_t last_packet_sent_size;
    
    bool server_may_not_support_options;   // if errors are received this flag could be set to true
};

struct transfer_stats {
    bool transfer_complete;
    uint16_t expected_next_block;
    uint16_t last_block_received;
    size_t buffer_size;
    uint8_t *buffer;
};

static struct tftp_client_response tftp_connected_client_get(struct tftp_client client[static 1],
                                                             struct tftp_client_connection connection[static 1],
                                                             struct tftp_client_get_request get_request[static 1],
                                                             struct tftp_client_options *options);
static struct tftp_client_response tftp_connected_client_put(struct tftp_client client[static 1],
                                                             struct tftp_client_connection connection[static 1],
                                                             struct tftp_client_put_request request[static 1],
                                                             struct tftp_client_options *options);
static struct tftp_client_response tftp_connected_client_list(struct tftp_client client[static 1],
                                                              struct tftp_client_connection connection[static 1],
                                                              struct tftp_client_options *options);

static struct tftp_client_response handle_get_request(struct request request[static 1],
                                                      struct tftp_client_connection connection[static 1],
                                                      FILE file_buffer[static 1]);

static void options_init(struct request request[static 1], struct tftp_client_options *options);

static bool send_read_request(struct request request[static 1]);
static bool receive_file(struct request request[static 1], FILE file_buffer[static 1]);
static bool write_tmp_to_output(const char output_path[static 1], FILE tmp[static 1], struct logger logger[static 1]);


static inline bool is_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]);
static void handle_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]);

static bool send_rrq(struct request request[static 1]);
static bool send_ack(struct request request[static 1], uint16_t received_block_number);
static bool send_error(struct request request[static 1], enum tftp_error_code error_code, const char *error_message);
static bool retransmit_last_packet_sent(struct request request[static 1]);

static bool send_rrq_packet(struct request request[static 1], const struct tftp_rrq_packet rrq_packet[static 1], size_t rrq_packet_size);
static bool send_ack_packet(struct request request[static 1], const struct tftp_ack_packet ack_packet[static 1]);
static bool send_error_packet(struct request request[static 1], const struct tftp_error_packet *error_packet, size_t error_packet_size);

static ssize_t receive_packet(struct request request[static 1],
                              struct transfer_stats stats[static 1],
                              struct server_sockaddr server_addr[static 1]);
static bool handle_packet(struct request request[static 1], FILE file[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);

static bool handle_error_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);
static bool handle_oack_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);
static bool handle_data_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size, FILE file[static 1]);

static inline size_t get_buffer_size(uint16_t block_size);

struct tftp_client_response tftp_client_get(struct tftp_client client[static 1],
                                            struct tftp_client_server_address server_address[static 1],
                                            struct tftp_client_get_request request[static 1],
                                            struct tftp_client_options *options) {
    struct tftp_client_connection connection;
    if (!tftp_client_connection_init(&connection, server_address->host, server_address->port, client->logger)) {
        return (struct tftp_client_response) {
            .success = false,
            .server_may_not_support_options = false,
        };
    }
    struct tftp_client_response result = tftp_connected_client_get(client, &connection, request, options);
    tftp_client_connection_destroy(&connection, client->logger);
    return result;
}

struct tftp_client_response tftp_client_put(struct tftp_client client[static 1],
                                            struct tftp_client_server_address server_address[static 1],
                                            struct tftp_client_put_request request[static 1],
                                            struct tftp_client_options *options) {
    struct tftp_client_connection connection;
    if (!tftp_client_connection_init(&connection, server_address->host, server_address->port, client->logger)) {
        return (struct tftp_client_response) {
            .success = false,
            .server_may_not_support_options = false,
        };
    }
    struct tftp_client_response result = tftp_connected_client_put(client, &connection, request, options);
    tftp_client_connection_destroy(&connection, client->logger);
    return result;
}

struct tftp_client_response tftp_client_list(struct tftp_client client[static 1],
                                             struct tftp_client_server_address server_address[static 1],
                                             struct tftp_client_options *options) {
    struct tftp_client_connection connection;
    if (!tftp_client_connection_init(&connection, server_address->host, server_address->port, client->logger)) {
        return (struct tftp_client_response) {
            .success = false,
            .server_may_not_support_options = false,
        };
    }
    struct tftp_client_response result = tftp_connected_client_list(client, &connection, options);
    tftp_client_connection_destroy(&connection, client->logger);
    return result;
}

struct tftp_client_response tftp_connected_client_get(struct tftp_client client[static 1],
                                                      struct tftp_client_connection connection[static 1],
                                                      struct tftp_client_get_request get_request[static 1],
                                                      struct tftp_client_options *options) {
    struct request request = {
        .logger = client->logger,
        .connection = connection,
        .client_stats_callback = client->client_stats_callback,
        .retries = client->retries,
        .request_type = REQUEST_GET,
        .details.get.mode = get_request->mode,
        .details.get.mode_str = tftp_mode_to_string(get_request->mode),
        .details.get.filename = get_request->filename,
        .details.get.output_path = get_request->output == nullptr ? get_request->filename : get_request->output,
    };
    options_init(&request, options);
    if (!tftp_client_connection_set_recv_timeout(connection, request.options.timeout_s, request.logger)) {
        goto fail;
    }
    tftp_client_stats_init(&request.stats, request.logger);
    FILE* tmp = tmpfile();
    if (tmp == nullptr) {
        logger_log_error(request.logger, "Could not create temporary file. %s", strerror(errno));
        goto fail;
    }
    struct tftp_client_response response = handle_get_request(&request, connection, tmp);
    if (!response.success) {
        fclose(tmp);
        return response;
    }
    if (!write_tmp_to_output(request.details.get.output_path, tmp, request.logger)) {
        fclose(tmp);
        return (struct tftp_client_response) {
            .success = false,
            .server_may_not_support_options = false,
        };
    }
    if (fclose(tmp) == EOF) {
        logger_log_error(request.logger, "Failed to close the temporary file. %s", strerror(errno));
    }
    return response;
fail:
    return (struct tftp_client_response) {
        .success = false,
        .server_may_not_support_options = request.server_may_not_support_options,
    };
}

static struct tftp_client_response tftp_connected_client_put(struct tftp_client client[static 1],
                                                             struct tftp_client_connection connection[static 1],
                                                             struct tftp_client_put_request request[static 1],
                                                             struct tftp_client_options *options) {
    return (struct tftp_client_response) {
        .success = false,
        .server_may_not_support_options = false,
    };
}

static struct tftp_client_response tftp_connected_client_list(struct tftp_client client[static 1],
                                                              struct tftp_client_connection connection[static 1],
                                                              struct tftp_client_options *options) {
    return (struct tftp_client_response) {
        .success = false,
        .server_may_not_support_options = false,
    };
}

static void options_init(struct request request[static 1], struct tftp_client_options *options) {
    if (options != nullptr) {
        request->options.timeout_s = options->timeout_s;
        request->options.block_size = options->block_size;
        request->options.window_size = options->window_size;
        request->options.use_tsize = options->use_tsize;
        request->options.use_adaptive_timeout = options->use_adaptive_timeout;
    }
    
    if (request->options.timeout_s == 0) {
        request->options.timeout_s = 2;
        request->options.is_timeout_s_default = true;
    }
    else {
        sprintf((char *) request->options.timeout_s_str, "%hhu", request->options.timeout_s);
    }
    if (request->options.block_size == 0) {
        request->options.block_size = tftp_default_blksize;
        request->options.is_block_size_default = true;
    }
    else {
        sprintf((char *) request->options.block_size_str, "%hu", request->options.block_size);
    }
    if (request->options.window_size == 0) {
        request->options.window_size = tftp_default_window_size;
        request->options.is_window_size_default = true;
    }
    else {
        sprintf((char *) request->options.window_size_str, "%hu", request->options.window_size);
    }
    sprintf((char *) request->options.tsize_str, "0");
    
    memcpy(request->options.options,
           &(struct tftp_option[TFTP_OPTION_TOTAL_OPTIONS]) {
               [TFTP_OPTION_BLKSIZE] = {.is_active = !request->options.is_block_size_default, .value = (const char *) request->options.block_size_str},
               [TFTP_OPTION_TIMEOUT] = {.is_active = !request->options.is_timeout_s_default, .value = (const char *) request->options.timeout_s_str},
               [TFTP_OPTION_TSIZE] = {.is_active = request->options.use_tsize, .value = (const char *) request->options.tsize_str},
               [TFTP_OPTION_WINDOWSIZE] = {.is_active = !request->options.is_window_size_default, . value = (const char *) request->options.window_size_str},
           },
           sizeof request->options.options);
    for (size_t i = 0; i < TFTP_OPTION_TOTAL_OPTIONS; i++) {
        if (request->options.options[i].is_active) {
            request->use_options = true;
            break;
        }
    }
    if (request->use_options) {
        tftp_format_options(request->options.options, request->options.options_str);
    }
}

static struct tftp_client_response handle_get_request(struct request request[static 1],
                                                      struct tftp_client_connection connection[static 1],
                                                      FILE file_buffer[static 1]) {
    char server_addr_str[INET6_ADDRSTRLEN];
    uint16_t server_port;
    if (inet_ntop_address((struct sockaddr *) &connection->server_addr.sockaddr, server_addr_str, &server_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_info(request->logger, "Getting file %s from unresolved server name [%s]",
                        request->details.get.filename,
                        request->details.get.mode_str);
    }
    else {
        logger_log_info(request->logger, "Getting file %s from %s:%hu [%s]",
                        request->details.get.filename,
                        server_addr_str,
                        server_port,
                        request->details.get.mode_str);
    }
    if (!send_read_request(request)) {
        goto fail;
    }
    if (!receive_file(request, file_buffer)) {
        goto fail;
    }
    if (request->client_stats_callback != nullptr) {
        request->client_stats_callback(&request->stats);
    }
    if (request->options.use_tsize && (request->stats.file_bytes_received != request->options.tsize)) {
        logger_log_error(request->logger, "Received file size does not match the expected size.");
        goto fail;
    }
    return (struct tftp_client_response) {
        .success = true,
        .server_may_not_support_options = false,
    };
fail:
    return (struct tftp_client_response) {
        .success = false,
        .server_may_not_support_options = request->server_may_not_support_options,
    };
}

static bool send_read_request(struct request request[static 1]) {
    if (!send_rrq(request)) {
        return false;
    }
    if (getsockname(request->connection->sockfd, (struct sockaddr *) &request->connection->sock_addr_storage, &request->connection->sock_addr_len) == -1) {
        logger_log_error(request->logger, "Failed to get the socket name. %s", strerror(errno));
        return false;
    }
    if (inet_ntop_address((struct sockaddr *) &request->connection->sock_addr_storage, request->connection->sock_addr_str, &request->connection->sock_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_debug(request->logger, "Bound to unresolved address");
    }
    else {
        logger_log_debug(request->logger, "Bound to %s:%hu", request->connection->sock_addr_str, request->connection->sock_port);
    }
    return true;
}

static bool receive_file(struct request request[static 1], FILE file_buffer[static 1]) {
    bool first_receive = true;
    struct transfer_stats stats = {
        .expected_next_block = 1,
        .buffer_size = get_buffer_size(request->options.block_size),
    };
    stats.buffer = malloc(stats.buffer_size);
    if (stats.buffer == nullptr) {
        logger_log_error(request->logger, "Failed to allocate memory for the buffer. %s", strerror(errno));
        return false;
    }
    while (!stats.transfer_complete) {
        struct server_sockaddr server_addr = {
            .socklen = sizeof server_addr.sockaddr,
        };
        ssize_t bytes_received = receive_packet(request, &stats, &server_addr);
        if (bytes_received == -1) {
            free(stats.buffer);
            return false;
        }
        if (first_receive) {
            request->connection->server_addr.sockaddr = server_addr.sockaddr;
            request->connection->server_addr.socklen = server_addr.socklen;
            first_receive = false;
        }
        if (is_unexpected_peer(request, &server_addr)) {
            handle_unexpected_peer(request, &server_addr);
            continue;
        }
        if (!handle_packet(request, file_buffer, &stats, bytes_received)) {
            free(stats.buffer);
            return false;
        }
    }
    free(stats.buffer);
    return true;
}

static inline bool is_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]) {
    struct server_sockaddr *expected = &request->connection->server_addr;
    struct server_sockaddr *actual = server_addr;
    return expected->socklen != actual->socklen ||
           memcmp(&expected->sockaddr, &actual->sockaddr, expected->socklen) != 0;
}

static void handle_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]) {
    struct server_sockaddr *expected = &request->connection->server_addr;
    struct server_sockaddr *actual = server_addr;
    char expected_addr_str[INET6_ADDRSTRLEN];
    uint16_t expected_port;
    bool expected_addr_string_resolved = (inet_ntop_address((struct sockaddr *) &expected->sockaddr, expected_addr_str, &expected_port) != nullptr);
    if (!expected_addr_string_resolved) {
        logger_log_warn(request->logger, "Could not translate expected address to a string representation. %s", strerror(errno));
    }
    char actual_addr_str[INET6_ADDRSTRLEN];
    uint16_t actual_port;
    bool actual_addr_string_resolved = (inet_ntop_address((struct sockaddr *) &actual->sockaddr, actual_addr_str, &actual_port) != nullptr);
    if (!actual_addr_string_resolved) {
        logger_log_warn(request->logger, "Could not translate actual address to a string representation. %s", strerror(errno));
    }
    if (expected_addr_string_resolved && actual_addr_string_resolved) {
        logger_log_warn(request->logger, "Unexpected peer: '%s:%d', expected peer: '%s:%d'. Ignoring packet.", actual_addr_str, actual_port, expected_addr_str, expected_port);
    }
    else if (expected_addr_string_resolved) {
        logger_log_warn(request->logger, "Unexpected peer: 'unresolved', expected peer: '%s:%d'. Ignoring packet.", expected_addr_str, expected_port);
    }
    else if (actual_addr_string_resolved) {
        logger_log_warn(request->logger, "Unexpected peer: '%s:%d', expected peer: 'unresolved'. Ignoring packet.", actual_addr_str, actual_port);
    }
    else {
        logger_log_warn(request->logger, "Unexpected peer: 'unresolved', expected peer: 'unresolved'. Ignoring packet.");
    }
    const struct tftp_error_packet *error_packet = tftp_error_packet_info[TFTP_ERROR_UNKNOWN_TRANSFER_ID].packet;
    size_t error_packet_size = tftp_error_packet_info[TFTP_ERROR_UNKNOWN_TRANSFER_ID].size;
    ssize_t bytes_sent = sendto(request->connection->sockfd,
                                error_packet,
                                error_packet_size,
                                0,
                                (struct sockaddr *) &actual->sockaddr,
                                actual->socklen);
    if (bytes_sent < 0) {
        logger_log_warn(request->logger, "Failed to send ERROR packet. %s", strerror(errno));
    }
    else {
        enum tftp_error_code error_code = ntohs(error_packet->error_code);
        const char *error_message = (const char *) error_packet->error_message;
        logger_log_trace(request->logger, "Sent ERROR <code=%d, message=%s>", error_code, error_message);
    }
}

static bool write_tmp_to_output(const char output_path[static 1], FILE tmp[static 1], struct logger logger[static 1]) {
    int tmp_fd = fileno(tmp);
    off64_t file_size = ftello64(tmp);
    rewind(tmp);
    if (tmp_fd < 0) {
        logger_log_error(logger, "Could not get temporary file descriptor. %s", strerror(errno));
        return false;
    }
    int out_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        logger_log_error(logger, "Could not open file %s. %s", output_path, strerror(errno));
        return false;
    }
    off64_t offset = 0;
    while (offset < file_size) {
        if (sendfile64(out_fd, tmp_fd, &offset, file_size - offset) == -1) {
            logger_log_error(logger, "Could not copy temporary file into output file. %s", strerror(errno));
            close(out_fd);
            return false;
        }
    }
    if (close(out_fd) < 0) {
        logger_log_error(logger, "Could not close output file. %s", strerror(errno));
    }
    return true;
}

static bool send_rrq(struct request request[static 1]) {
    struct tftp_rrq_packet rrq_packet;
    ssize_t rrq_packet_size;
    rrq_packet_size = tftp_rrq_packet_init(&rrq_packet,
                                           strlen(request->details.get.filename) + 1,
                                           request->details.get.filename,
                                           request->details.get.mode,
                                           request->options.options);
    if (rrq_packet_size < 0) {
        logger_log_error(request->logger, "Failed to create RRQ packet");
        return false;
    }
    if (!send_rrq_packet(request, &rrq_packet, rrq_packet_size)) {
        return false;
    }
    request->last_packet_sent.rrq_packet = rrq_packet;
    request->last_packet_sent_size = rrq_packet_size;
    return true;
}

static bool send_ack(struct request request[static 1], uint16_t received_block_number) {
    struct tftp_ack_packet ack_packet;
    ssize_t ack_packet_size = sizeof ack_packet;
    tftp_ack_packet_init(&ack_packet, received_block_number);
    if (!send_ack_packet(request, &ack_packet)) {
        return false;
    }
    request->last_packet_sent.ack_packet = ack_packet;
    request->last_packet_sent_size = ack_packet_size;
    return true;
}

static bool send_error(struct request request[static 1], enum tftp_error_code error_code, const char *error_message) {
    if (error_code == TFTP_ERROR_NOT_DEFINED) {
        logger_log_error(request->logger, "Undefined error code are still not supported.");
        (void) error_message;
        return false;
    }
    const struct tftp_error_packet *error_packet = tftp_error_packet_info[error_code].packet;
    size_t error_packet_size = tftp_error_packet_info[error_code].size;
    if (!send_error_packet(request, error_packet, error_packet_size)) {
        return false;
    }
    // since error packets are not acknowledged nor retransmitted, they don't need to be stored
    return true;
}

static bool retransmit_last_packet_sent(struct request request[static 1]) {
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(&request->last_packet_sent);
    bool ret = false;
    switch (opcode) {
        case TFTP_OPCODE_RRQ:
            ret = send_rrq_packet(request, &request->last_packet_sent.rrq_packet, request->last_packet_sent_size);
            break;
        case TFTP_OPCODE_ACK:
            ret = send_ack_packet(request, &request->last_packet_sent.ack_packet);
            break;
        default:
            logger_log_error(request->logger, "Last packet sent is of an unexpected packet type %d", opcode);
            break;
    }
    return ret;
}

static bool send_rrq_packet(struct request request[static 1], const struct tftp_rrq_packet rrq_packet[static 1], size_t rrq_packet_size) {
    ssize_t bytes_sent = sendto(request->connection->sockfd,
                                rrq_packet,
                                rrq_packet_size,
                                0,
                                (struct sockaddr *) &request->connection->server_addr.sockaddr,
                                request->connection->server_addr.socklen);
    if (bytes_sent < 0) {
        logger_log_error(request->logger, "Failed to send RRQ packet. %s", strerror(errno));
        return false;
    }
    logger_log_trace(request->logger, "Sent RRQ <file=%s, mode=%s%s%s>", request->details.get.filename, request->details.get.mode_str, request->use_options ? ", options=" : "", request->use_options ? request->options.options_str : "");
    return true;
}

static bool send_ack_packet(struct request request[static 1], const struct tftp_ack_packet ack_packet[static 1]) {
    ssize_t bytes_sent = sendto(request->connection->sockfd,
                                ack_packet,
                                sizeof *ack_packet,
                                0,
                                (struct sockaddr *) &request->connection->server_addr.sockaddr,
                                request->connection->server_addr.socklen);
    if (bytes_sent < 0) {
        logger_log_error(request->logger, "Failed to send ACK packet. %s", strerror(errno));
        return false;
    }
    uint16_t received_block_number = ntohs(ack_packet->block_number);
    logger_log_trace(request->logger, "Sent ACK <block=%d>", received_block_number);
    return true;
}

static bool send_error_packet(struct request request[static 1], const struct tftp_error_packet *error_packet, size_t error_packet_size) {
    // TODO: check for null pointer or just UB?
    ssize_t bytes_sent = sendto(request->connection->sockfd,
                                error_packet,
                                error_packet_size,
                                0,
                                (struct sockaddr *) &request->connection->server_addr.sockaddr,
                                request->connection->server_addr.socklen);
    if (bytes_sent < 0) {
        logger_log_error(request->logger, "Failed to send ERROR packet. %s", strerror(errno));
        return false;
    }
    enum tftp_error_code error_code = ntohs(error_packet->error_code);
    const char *error_message = (const char *) error_packet->error_message;
    logger_log_trace(request->logger, "Sent ERROR <code=%s, message=%s>", error_code, error_message);
    return true;
}

static ssize_t receive_packet(struct request request[static 1],
                              struct transfer_stats stats[static 1],
                              struct server_sockaddr server_addr[static 1]) {
    auto total_attempts = 1 + request->retries;
    uint8_t retransmits = 0;
    ssize_t bytes_received = 0;
    while (retransmits < total_attempts) {
        bytes_received = recvfrom(request->connection->sockfd,
                                  stats->buffer,
                                  stats->buffer_size,
                                  0,
                                  (struct sockaddr *) &server_addr->sockaddr,
                                  &server_addr->socklen);
        if (bytes_received != -1) {
            break;
        }
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            logger_log_error(request->logger, "Failed to receive data. %s", strerror(errno));
            return -1;
        }
        if (retransmits == request->retries) {
            logger_log_error(request->logger, "Transfer timed out after %d of %d retransmissions.", retransmits, request->retries);
            return -1;
        }
        logger_log_warn(request->logger, "Receive timed out. Retransmitting last packet.");
        if (!retransmit_last_packet_sent(request)) {
            return -1;
        }
        retransmits++;
    }
    return bytes_received;
}

static bool handle_packet(struct request request[static 1], FILE file[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(stats->buffer);
    switch (opcode) {
        case TFTP_OPCODE_OACK:
            return handle_oack_packet(request, stats, packet_size);
        case TFTP_OPCODE_DATA:
            return handle_data_packet(request, stats, packet_size, file);
        case TFTP_OPCODE_ERROR:
            return handle_error_packet(request, stats, packet_size);
        default:
            logger_log_error(request->logger, "Unexpected opcode %d", opcode);
            return false;
    }
}

static bool handle_error_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    struct tftp_error_packet *error_packet = (struct tftp_error_packet *) stats->buffer;
    uint16_t error_code = ntohs(error_packet->error_code);
    const char *error_message = (const char *) error_packet->error_message;
    logger_log_error(request->logger, "Received error %d: %s", error_code, error_message);
    if (stats->last_block_received == 0 && request->use_options) {
        request->server_may_not_support_options = true;
    }
    return false;
}

static bool handle_oack_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    struct tftp_oack_packet *oack_packet = (struct tftp_oack_packet *) stats->buffer;
    struct tftp_option ackd_options[TFTP_OPTION_TOTAL_OPTIONS] = {};
    char ackd_options_str[tftp_option_formatted_string_max_size];
    bool contains_unrequested_options = false;
    uint16_t block_size = request->options.block_size;
    uint16_t timeout = request->options.timeout_s;
    uint16_t window_size = request->options.window_size;
    tftp_parse_options(ackd_options, packet_size - sizeof *oack_packet, (char *) oack_packet->options_values);
    tftp_format_options(ackd_options, ackd_options_str);
    logger_log_trace(request->logger, "Received OACK %s", ackd_options_str);
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (ackd_options[o].is_active) {
            if (!request->options.options[o].is_active) {
                contains_unrequested_options = true;
                break;
            }
            switch (o) {
                case TFTP_OPTION_BLKSIZE:
                    block_size = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TIMEOUT:
                    timeout = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TSIZE:
                    request->options.tsize = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_WINDOWSIZE:
                    window_size = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                default: // unreachable
                    break;
            }
        }
    }
    if (contains_unrequested_options) {
        logger_log_error(request->logger, "Received OACK with unrequested options.");
        const struct tftp_error_packet_info *error_info = &tftp_error_packet_info[TFTP_ERROR_INVALID_OPTIONS];
        if (sendto(request->connection->sockfd, error_info->packet, error_info->size, 0, (struct sockaddr *) &request->connection->server_addr.sockaddr, request->connection->server_addr.socklen) < 0) {
            logger_log_error(request->logger, "Failed to send ERROR packet. %s", strerror(errno));
        }
        return false;
    }
    if (block_size != request->options.block_size || window_size != request->options.window_size) {
        request->options.block_size = block_size;
        request->options.window_size = window_size;
        stats->buffer_size = get_buffer_size(request->options.block_size);
        uint8_t *new_buffer = realloc(stats->buffer, stats->buffer_size);
        if (new_buffer == nullptr) {
            logger_log_error(request->logger, "Failed to reallocate memory for the buffer. %s", strerror(errno));
            return false;
        }
        stats->buffer = new_buffer;
    }
    if (timeout != request->options.timeout_s) {
        request->options.timeout_s = timeout;
        struct timeval new_timeout = {.tv_sec = request->options.timeout_s,};
        if (setsockopt(request->connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, &new_timeout, sizeof new_timeout) == -1) {
            logger_log_error(request->logger, "Could not set listener socket options for the server. %s", strerror(errno));
            return false;
        }
    }
    if (!send_ack(request, stats->last_block_received)) {
        return false;
    }
    return true;
}

static bool handle_data_packet(struct request request[static 1], struct transfer_stats stats[static 1], ssize_t packet_size, FILE file[static 1]) {
    struct tftp_data_packet *data_packet = (struct tftp_data_packet *) stats->buffer;
    stats->last_block_received = ntohs(data_packet->block_number);
    if (stats->last_block_received == stats->expected_next_block) {
        stats->expected_next_block++;
        size_t block_size = packet_size - sizeof *data_packet;
        fwrite(data_packet->data, 1, block_size, file);
        request->stats.file_bytes_received += block_size;
        logger_log_trace(request->logger, "Received DATA <block=%d, size=%zu bytes>", stats->last_block_received, block_size);
        if (!send_ack(request, stats->last_block_received)) {
            return false;
        }
        if (block_size < request->options.block_size) {
            stats->transfer_complete = true;
        }
    }
    return true;
}

static inline size_t get_buffer_size(uint16_t block_size) {
    static size_t min_buffer_size = sizeof(struct tftp_rrq_packet) - 4;
    size_t required_buffer_size = sizeof(struct tftp_data_packet) + block_size;
    return required_buffer_size < min_buffer_size ? min_buffer_size : required_buffer_size;
}
