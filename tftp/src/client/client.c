#include <buracchi/tftp/client.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tftp.h>

#include "connection.h"
#include "stats.h"
#include "../adaptive_timeout.h"
#include "../utils/inet.h"
#include "../utils/time.h"

#define UINT8_STRLEN 4
#define UINT16_STRLEN 6
#define SIZE_STRLEN 21

constexpr uint8_t default_timeout_s = 2;

enum request_type {
    REQUEST_GET,
    REQUEST_PUT,
};

struct get_request {
    const char *filename;
    enum tftp_mode mode;
    const char *mode_str;
};

struct put_request {
    const char *filename;
    enum tftp_mode mode;
    const char *mode_str;
};

struct options {
    struct tftp_option options[TFTP_OPTION_TOTAL_OPTIONS];
    char options_str[tftp_option_formatted_string_max_size];
    
    uint8_t timeout_s;
    char *timeout_s_str[9];
    
    uint16_t block_size;
    char *block_size_str[UINT16_STRLEN];
    
    uint16_t window_size;
    char *window_size_str[UINT16_STRLEN];
    
    bool use_tsize;
    size_t tsize;
    char *tsize_str[SIZE_STRLEN];
    
    bool use_adaptive_timeout;
};

struct request {
    struct logger *logger;
    struct connection *connection;
    struct tftp_client_stats stats;
    
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
        struct tftp_wrq_packet wrq_packet;
        struct tftp_ack_packet ack_packet;
        // error packet are not acknowledged or retransmitted, so they don't need to be stored
    } last_packet_sent;
    size_t last_packet_sent_size;
    
    uint8_t *packet_recv_buffer;
    size_t packet_recv_buffer_size;
    
    bool server_may_not_support_options;   // if errors are received this flag could be set to true
};

enum receive_packet_error: ssize_t {
    RECEIVE_PACKET_ERROR = -1,
    RECEIVE_PACKET_TIMEOUT = -2,
};

static inline bool is_in_range(uint16_t n, uint16_t begin, uint16_t end) {
    return begin <= end ? (begin <= n && n <= end) : (begin <= n || n <= end);
}

static struct tftp_client_response handle_get_request(struct request request[static 1],
                                                      struct connection connection[static 1],
                                                      FILE file_buffer[static 1]);

static struct tftp_client_response handle_put_request(struct request request[static 1],
                                                      struct connection connection[static 1],
                                                      FILE file[static 1]);

static bool options_init(struct options result[static 1],
                         enum request_type request_type,
                         struct tftp_client_options *options);

static bool send_read_request(struct request request[static 1]);

static bool send_write_request(struct request request[static 1]);

static bool receive_file(struct request request[static 1], FILE file[static 1]);

static bool send_file(struct request request[static 1], FILE file[static 1]);

static void handle_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]);

static bool send_rrq(struct request request[static 1]);

static bool send_wrq(struct request request[static 1]);

static bool send_ack(struct request request[static 1], uint16_t received_block_number);

static bool send_error(struct request request[static 1], enum tftp_error_code error_code, const char *error_message);

static bool retransmit_last_packet_sent(struct request request[static 1]);

static bool send_rrq_packet(struct request request[static 1], const struct tftp_rrq_packet rrq_packet[static 1],
                            size_t rrq_packet_size);

static bool send_wrq_packet(struct request request[static 1], const struct tftp_wrq_packet wrq_packet[static 1],
                            size_t wrq_packet_size);

static bool send_ack_packet(struct request request[static 1], const struct tftp_ack_packet ack_packet[static 1]);

static bool send_error_packet(struct request request[static 1], const struct tftp_error_packet *error_packet,
                              size_t error_packet_size);

static ssize_t receive_packet(struct request request[static 1], struct server_sockaddr server_addr[static 1]);

static inline void log_error_packet_received(struct logger logger[static 1], struct tftp_error_packet *error_packet,
                                             size_t packet_size);

static bool handle_oack_packet(struct request request[static 1], ssize_t packet_size);

static inline size_t tftp_packet_buffer_size(uint16_t required_block_size);

struct tftp_client_response tftp_client_read(struct logger logger[static 1],
                                             uint8_t retries,
                                             const char host[static 1],
                                             const char port[static 1],
                                             const char filename[static 1],
                                             enum tftp_mode mode,
                                             struct tftp_client_options *options,
                                             FILE dest[static 1]) {
    struct connection connection;
    if (!connection_init(&connection, host, port, logger)) {
        return (struct tftp_client_response) {
            .is_success = false,
            .error.server_may_not_support_options = false,
        };
    }
    struct request request = {
        .logger = logger,
        .connection = &connection,
        .retries = retries,
        .request_type = REQUEST_GET,
        .details.get.mode = mode,
        .details.get.mode_str = tftp_mode_to_string(mode),
        .details.get.filename = filename,
        .packet_recv_buffer = nullptr,
    };
    request.use_options = options_init(&request.options, request.request_type, options);
    if (!connection_set_recv_timeout(&connection, request.options.timeout_s, request.logger)) {
        goto fail;
    }
    stats_init(&request.stats);
    struct tftp_client_response response = handle_get_request(&request, &connection, dest);
    connection_destroy(&connection, logger);
    return response;
fail:
    connection_destroy(&connection, logger);
    return (struct tftp_client_response) {
        .is_success = false,
        .error.server_may_not_support_options = request.server_may_not_support_options,
    };
}

struct tftp_client_response tftp_client_write(struct logger logger[static 1],
                                              uint8_t retries,
                                              const char host[static 1],
                                              const char port[static 1],
                                              const char filename[static 1],
                                              enum tftp_mode mode,
                                              struct tftp_client_options *options,
                                              FILE dest[static 1]) {
    struct connection connection;
    if (!connection_init(&connection, host, port, logger)) {
        return (struct tftp_client_response) {
            .is_success = false,
            .error.server_may_not_support_options = false,
        };
    }
    struct request request = {
        .logger = logger,
        .connection = &connection,
        .retries = retries,
        .request_type = REQUEST_PUT,
        .details.put.mode = mode,
        .details.put.mode_str = tftp_mode_to_string(mode),
        .details.put.filename = filename,
    };
    request.use_options = options_init(&request.options, request.request_type, options);
    if (!connection_set_recv_timeout(&connection, request.options.timeout_s, request.logger)) {
        goto fail;
    }
    stats_init(&request.stats);
    struct tftp_client_response response = handle_put_request(&request, &connection, dest);
    connection_destroy(&connection, logger);
    return response;
fail:
    connection_destroy(&connection, logger);
    return (struct tftp_client_response) {
        .is_success = false,
        .error.server_may_not_support_options = request.server_may_not_support_options,
    };
}

static bool options_init(struct options result[static 1],
                         enum request_type request_type,
                         struct tftp_client_options *options) {
    if (options == nullptr) {
        return false;
    }
    const bool is_timeout_required = options->timeout_s != nullptr
                                     && *options->timeout_s != 0;
    const bool is_block_size_required = options->block_size != nullptr
                                        && *options->block_size >= 8
                                        && *options->block_size <= 65464;
    const bool is_window_size_required = options->window_size != nullptr
                                         && *options->window_size != 0
                                         && request_type != REQUEST_PUT;
    const bool is_tsize_required = options->use_tsize;
    const bool is_adaptive_timeout_required = options->use_adaptive_timeout && request_type == REQUEST_GET;
    
    result->timeout_s = is_timeout_required ? *options->timeout_s : default_timeout_s;
    result->block_size = is_block_size_required ? *options->block_size : tftp_default_blksize;
    result->window_size = is_window_size_required ? *options->window_size : tftp_default_window_size;
    result->use_tsize = is_tsize_required;
    result->use_adaptive_timeout = options->use_adaptive_timeout;
    
    if (is_timeout_required && !is_adaptive_timeout_required) {
        sprintf((char *) result->timeout_s_str, "%hhu", result->timeout_s);
    }
    if (is_adaptive_timeout_required) {
        sprintf((char *) result->timeout_s_str, "adaptive");
    }
    if (is_block_size_required) {
        sprintf((char *) result->block_size_str, "%hu", result->block_size);
    }
    if (is_window_size_required) {
        sprintf((char *) result->window_size_str, "%hu", result->window_size);
    }
    // TODO: handle write request tsize option
    sprintf((char *) result->tsize_str, "0");
    
    memcpy(result->options,
           &(struct tftp_option[TFTP_OPTION_TOTAL_OPTIONS]) {
               [TFTP_OPTION_BLKSIZE] = {.is_active = is_block_size_required, .value = (const char *) result->block_size_str},
               [TFTP_OPTION_TIMEOUT] = {.is_active = is_timeout_required || is_adaptive_timeout_required, .value = (const char *) result->timeout_s_str},
               [TFTP_OPTION_TSIZE] = {.is_active = result->use_tsize, .value = (const char *) result->tsize_str},
               [TFTP_OPTION_WINDOWSIZE] = {.is_active = is_window_size_required, .value = (const char *) result->window_size_str},
               [TFTP_OPTION_READ_TYPE] = {.is_active = options != nullptr && options->is_read_type_list, .value = "directory"},
           },
           sizeof result->options);
    for (size_t i = 0; i < TFTP_OPTION_TOTAL_OPTIONS; i++) {
        if (result->options[i].is_active) {
            tftp_format_options(result->options, result->options_str);
            return true;
        }
    }
    return false;
}

static struct tftp_client_response handle_get_request(struct request request[static 1],
                                                      struct connection connection[static 1],
                                                      FILE file_buffer[static 1]) {
    struct sockaddr *server_addr = (struct sockaddr *) &connection->server_addr.sockaddr;
    char server_addr_str[INET6_ADDRSTRLEN];
    uint16_t server_port;
    const char *operation = (!request->use_options || !request->options.options[TFTP_OPTION_READ_TYPE].is_active) ? "Getting file" : "Listing directory";
    if (sockaddr_ntop(server_addr, server_addr_str, &server_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_info(request->logger, "%s %s from unresolved server name [%s]", operation, request->details.get.filename, request->details.get.mode_str);
    }
    else {
        logger_log_info(request->logger, "%s %s from %s:%hu [%s]", operation, request->details.get.filename, server_addr_str, server_port, request->details.get.mode_str);
    }
    if (!send_read_request(request)) {
        goto fail;
    }
    request->packet_recv_buffer_size = tftp_packet_buffer_size(request->options.block_size);
    request->packet_recv_buffer = malloc(request->packet_recv_buffer_size);
    if (request->packet_recv_buffer == nullptr) {
        logger_log_error(request->logger, "Failed to allocate memory for the receive buffer. %s", strerror(errno));
        goto fail;
    }
    if (!receive_file(request, file_buffer)) {
        free(request->packet_recv_buffer);
        goto fail;
    }
    free(request->packet_recv_buffer);
    if (request->options.use_tsize && (request->stats.file_bytes_transferred != request->options.tsize)) {
        logger_log_error(request->logger, "Received file size does not match the expected size.");
        goto fail;
    }
    return (struct tftp_client_response) {
        .is_success = true,
        .value = request->stats,
    };
fail:
    return (struct tftp_client_response) {
        .is_success = false,
        .error.server_may_not_support_options = request->server_may_not_support_options,
    };
}

static struct tftp_client_response handle_put_request(struct request request[static 1],
                                                      struct connection connection[static 1],
                                                      FILE file[static 1]) {
    uint8_t recv_buffer[tftp_oack_packet_max_size];
    request->packet_recv_buffer_size = tftp_oack_packet_max_size;
    request->packet_recv_buffer = recv_buffer;
    struct sockaddr *server_addr = (struct sockaddr *) &connection->server_addr.sockaddr;
    char server_addr_str[INET6_ADDRSTRLEN];
    uint16_t server_port;
    if (sockaddr_ntop(server_addr, server_addr_str, &server_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_info(request->logger, "Sending file %s from unresolved server name [%s]", request->details.get.filename, request->details.get.mode_str);
    }
    else {
        logger_log_info(request->logger, "Sending file %s from %s:%hu [%s]", request->details.get.filename, server_addr_str, server_port, request->details.get.mode_str);
    }
    auto max_attempts = 1 + request->retries;
    uint8_t retransmissions = 0;
    ssize_t bytes_received = 0;
    struct server_sockaddr server_addr_find_better_name = {
        .socklen = sizeof server_addr_find_better_name.sockaddr,
    };
    while (retransmissions < max_attempts) {
        if (!send_write_request(request)) {
            goto fail;
        }
        bytes_received = recvfrom(request->connection->sockfd,
                                  request->packet_recv_buffer,
                                  request->packet_recv_buffer_size,
                                  0,
                                  (struct sockaddr *) &server_addr_find_better_name.sockaddr,
                                  &server_addr_find_better_name.socklen);
        if (bytes_received != -1) {
            break;
        }
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            logger_log_error(request->logger, "Failed to receive data. %s", strerror(errno));
            goto fail;
        }
        if (retransmissions == request->retries) {
            logger_log_error(request->logger, "Transfer timed out after %d of %d retransmissions.", retransmissions, request->retries);
            request->server_may_not_support_options = request->use_options;
            goto fail;
        }
        logger_log_warn(request->logger, "Receive timed out. Retransmitting last packet.");
        retransmissions++;
    }
    request->connection->server_addr.sockaddr = server_addr_find_better_name.sockaddr;
    request->connection->server_addr.socklen = server_addr_find_better_name.socklen;
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(request->packet_recv_buffer);
    switch (opcode) {
        case TFTP_OPCODE_OACK:
            if (!request->use_options) {
                goto unexpected_opcode;
            }
            if (!handle_oack_packet(request, bytes_received)) {
                break;
            }
            break;
        case TFTP_OPCODE_ACK:
            struct tftp_ack_packet *ack_packet = (struct tftp_ack_packet *) request->packet_recv_buffer;
            uint16_t block_number = ntohs(ack_packet->block_number);
            if (block_number != 0) {
                logger_log_error(request->logger, "Received unexpected ACK <block=%d>.", block_number);
                break;
            }
            if (request->use_options) {
                request->use_options = false;
                request->options.block_size = tftp_default_blksize;
                request->options.timeout_s = default_timeout_s;
                request->options.use_tsize = false;
            }
            logger_log_trace(request->logger, "Received ACK <block=%d>", block_number);
            break;
        case TFTP_OPCODE_ERROR:
            log_error_packet_received(request->logger, (struct tftp_error_packet *) request->packet_recv_buffer, bytes_received);
            request->server_may_not_support_options = request->use_options;
            goto fail;
unexpected_opcode:
        default:
            logger_log_error(request->logger, "Unexpected opcode %d", opcode);
            goto fail;
    }
    if (!send_file(request, file)) {
        goto fail;
    }
    request->packet_recv_buffer = nullptr;
    return (struct tftp_client_response) {
        .is_success = true,
        .value = request->stats,
    };
fail:
    request->packet_recv_buffer = nullptr;
    return (struct tftp_client_response) {
        .is_success = false,
        .error.server_may_not_support_options = request->server_may_not_support_options,
    };
}

static bool send_read_request(struct request request[static 1]) {
    struct sockaddr *addr = (struct sockaddr *) &request->connection->sock_addr_storage;
    socklen_t *addr_len = &request->connection->sock_addr_len;
    if (!send_rrq(request)) {
        return false;
    }
    if (getsockname(request->connection->sockfd, addr, addr_len) == -1) {
        logger_log_error(request->logger, "Failed to get the socket name. %s", strerror(errno));
        return false;
    }
    if (sockaddr_ntop(addr, request->connection->sock_addr_str, &request->connection->sock_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_debug(request->logger, "Bound to unresolved address");
    }
    else {
        logger_log_debug(request->logger, "Bound to %s:%hu", request->connection->sock_addr_str, request->connection->sock_port);
    }
    return true;
}

static bool send_write_request(struct request request[static 1]) {
    struct sockaddr *addr = (struct sockaddr *) &request->connection->sock_addr_storage;
    socklen_t *addr_len = &request->connection->sock_addr_len;
    if (!send_wrq(request)) {
        return false;
    }
    if (getsockname(request->connection->sockfd, addr, addr_len) == -1) {
        logger_log_error(request->logger, "Failed to get the socket name. %s", strerror(errno));
        return false;
    }
    if (sockaddr_ntop(addr, request->connection->sock_addr_str, &request->connection->sock_port) == nullptr) {
        logger_log_warn(request->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        logger_log_debug(request->logger, "Bound to unresolved address");
    }
    else {
        logger_log_debug(request->logger, "Bound to %s:%hu", request->connection->sock_addr_str, request->connection->sock_port);
    }
    return true;
}

static bool receive_file(struct request request[static 1], FILE file[static 1]) {
    bool is_first_receive = true;
    bool transfer_complete = false;
    uint16_t expected_sequence_number = 1;
    while (!transfer_complete) {
        struct server_sockaddr server_addr = {
            .socklen = sizeof server_addr.sockaddr,
        };
        ssize_t bytes_received = receive_packet(request, &server_addr);
        if (bytes_received == 0) {
            logger_log_error(request->logger, "Received empty packet.");
            return false;
        }
        if (bytes_received < 0) {
            enum receive_packet_error error = (enum receive_packet_error) bytes_received;
            if (is_first_receive && error == RECEIVE_PACKET_TIMEOUT && request->use_options) {
                request->server_may_not_support_options = true;
            }
            return false;
        }
        if (is_first_receive) {
            request->connection->server_addr.sockaddr = server_addr.sockaddr;
            request->connection->server_addr.socklen = server_addr.socklen;
        }
        if (!sockaddr_equals(&request->connection->server_addr.sockaddr, &server_addr.sockaddr)) {
            handle_unexpected_peer(request, &server_addr);
            continue;
        }
        enum tftp_opcode opcode = tftp_get_opcode_unsafe(request->packet_recv_buffer);
        switch (opcode) {
            case TFTP_OPCODE_OACK:
                bool is_list_request = request->options.options[TFTP_OPTION_READ_TYPE].is_active;
                if (!is_first_receive) {
                    break;  // ignore out of order OACK packets
                }
                if (!handle_oack_packet(request, bytes_received)) {
                    return false;
                }
                if (is_list_request && !request->options.options[TFTP_OPTION_READ_TYPE].is_active) {
                    logger_log_error(request->logger, "Received OACK packet without directory listing option. Server may not support list requests.");
                    send_error(request, TFTP_ERROR_INVALID_OPTIONS, nullptr);
                    return false;
                }
                if (!send_ack(request, 0)) {
                    return false;
                }
                break;
            case TFTP_OPCODE_DATA:
                if (is_first_receive) {
                    if (request->use_options) {
                        logger_log_error(request->logger, "Received DATA packet before OACK packet, discarding options.");
                        request->use_options = false;
                        if (request->options.options[TFTP_OPTION_READ_TYPE].is_active) {
                            send_error(request, TFTP_ERROR_INVALID_OPTIONS, nullptr);
                            request->server_may_not_support_options = true;
                            return false;
                        }
                    }
                    request->options.window_size = tftp_default_window_size;
                    request->options.block_size = tftp_default_blksize;
                    request->options.timeout_s = default_timeout_s;
                    request->options.use_adaptive_timeout = false;
                    request->options.use_tsize = false;
                }
                struct tftp_data_packet *data_packet = (struct tftp_data_packet *) request->packet_recv_buffer;
                uint16_t block_number = ntohs(data_packet->block_number);
                if (block_number != expected_sequence_number) {
                    logger_log_debug(request->logger, "Received out of order DATA packet: expected block %hu, received block %hu. Retransmitting last ACK.", expected_sequence_number, block_number);
                    if (!retransmit_last_packet_sent(request)) {
                        return false;
                    }
                    break;  // ignore out of order DATA packets
                }
                expected_sequence_number++;
                size_t block_size = bytes_received - sizeof *data_packet;
                if (fwrite(data_packet->data, 1, block_size, file) != block_size) {
                    logger_log_error(request->logger, "Failed to write to file. %s", strerror(errno));
                    if (errno == ENOSPC) {
                        send_error(request, TFTP_ERROR_DISK_FULL, nullptr);
                    }
                    else {
                        send_error(request, TFTP_ERROR_NOT_DEFINED, nullptr);
                    }
                    return false;
                }
                request->stats.file_bytes_transferred += block_size;
                logger_log_trace(request->logger, "Received DATA <block=%d, size=%zu bytes>", block_number, block_size);
                if (!send_ack(request, block_number)) {
                    return false;
                }
                if (block_size < request->options.block_size) {
                    transfer_complete = true;
                }
                break;
            case TFTP_OPCODE_ERROR:
                log_error_packet_received(request->logger, (struct tftp_error_packet *) request->packet_recv_buffer, bytes_received);
                request->server_may_not_support_options = (is_first_receive && request->use_options);
                return false;
            default:
                logger_log_error(request->logger, "Unexpected opcode %d", opcode);
                return false;
        }
        is_first_receive = false;
    }
    return true;
}

static bool send_file(struct request request[static 1], FILE file[static 1]) {
    size_t last_packet_size = 0;
    uint16_t window_begin = 1;
    uint16_t next_sequence_number = 1;
    bool end_of_file = false;
    uint8_t retransmissions = 0;
    bool transfer_completed = false;
    
    const size_t data_packet_size = sizeof(struct tftp_data_packet) + request->options.block_size;
    uint8_t *data_packets = malloc(request->options.window_size * data_packet_size);
    if (data_packets == nullptr) {
        logger_log_error(request->logger, "Failed to allocate memory for the data packet. %s", strerror(errno));
        // TODO: send error packet
        return false;
    }
    
    double rto;
    struct timespec start_time;
    struct adaptive_timeout adaptive_timeout;
    if (request->options.use_adaptive_timeout) {
        adaptive_timeout_init(&adaptive_timeout);
        rto = timespec_to_double(adaptive_timeout.rto);
    } else {
        rto = request->options.timeout_s;
    }
    
    while (!transfer_completed) {
        /* --- Sending Phase: Fill the window --- */
        while (!end_of_file && is_in_range(next_sequence_number, window_begin, window_begin + request->options.window_size - 1)) {
            const size_t offset = (((uint16_t) (next_sequence_number - 1)) % request->options.window_size) * data_packet_size;
            auto packet = (struct tftp_data_packet *) (data_packets + offset);
            
            size_t bytes_read = fread(packet->data, 1, request->options.block_size, file);
            if (bytes_read < request->options.block_size) {
                if (ferror(file)) {
                    logger_log_error(request->logger, "Failed to read from file. %s", strerror(errno));
                    // TODO: send an error packet
                    //send_error(request, TFTP_ERROR_NOT_DEFINED, strerror(errno));
                    goto fail;
                }
                end_of_file = true;
            }
            last_packet_size = sizeof *packet + bytes_read;
            tftp_data_packet_init(packet, next_sequence_number);
            logger_log_trace(request->logger, "Created DATA packets.");
            ssize_t bytes_sent = sendto(request->connection->sockfd,
                                        packet,
                                        last_packet_size,
                                        0,
                                        (struct sockaddr *) &request->connection->server_addr.sockaddr,
                                        request->connection->server_addr.socklen);
            if (bytes_sent == -1) {
                logger_log_error(request->logger, "Failed to send data packet. %s", strerror(errno));
                goto fail;
            }
            logger_log_trace(request->logger, "Sent DATA packet <block=%d, size=%zu bytes>.", next_sequence_number, bytes_read);
            request->stats.file_bytes_transferred += bytes_read;
            if (window_begin == next_sequence_number) {
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                if (request->options.use_adaptive_timeout) {
                    adaptive_timeout_start_timer(&adaptive_timeout);
                    adaptive_timeout.starting_block_number = next_sequence_number;
                }
            }
            next_sequence_number++;
        }
        
        /* --- Receiving Phase: Wait for ACK or timeout --- */
        bool is_timeout = false;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed_time = timespec_to_double(now) - timespec_to_double(start_time);
        double remaining_time = rto - elapsed_time;
        
        ssize_t bytes_received = 0;
        struct server_sockaddr server_addr;
        server_addr.socklen = sizeof(server_addr.sockaddr);
        
        if (remaining_time <= 0) {
            is_timeout = true;
        }
        else {
            struct timeval tv = double_to_timeval(remaining_time);
            if (setsockopt(request->connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                logger_log_error(request->logger, "Failed to set socket timeout. %s", strerror(errno));
                goto fail;
            }
            bytes_received = recvfrom(request->connection->sockfd,
                                      request->packet_recv_buffer,
                                      request->packet_recv_buffer_size,
                                      0,
                                      (struct sockaddr *) &server_addr.sockaddr,
                                      &server_addr.socklen);
            if (bytes_received < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    is_timeout = true;
                }
                else {
                    logger_log_error(request->logger, "Failed to receive data. %s", strerror(errno));
                    goto fail;
                }
            }
            else if (!sockaddr_equals(&request->connection->server_addr.sockaddr, &server_addr.sockaddr)) {
                handle_unexpected_peer(request, &server_addr);
                continue;
            }
        }
        
        if (is_timeout) {
            if (retransmissions > request->retries) {
                logger_log_error(request->logger, "Transfer timed out after %d retransmissions.", request->retries);
                //TODO: send an error packet
                //send_error(request, TFTP_ERROR_NOT_DEFINED, "Transfer timed out.");
                goto fail;
            }
            retransmissions++;
            logger_log_debug(request->logger, "Timeout occurred. Retransmission no %d of the packets in window [%d, %d]..", retransmissions, window_begin, (uint16_t) (next_sequence_number - 1));
            if (request->options.use_adaptive_timeout) {
                adaptive_timeout_cancel_timer(&adaptive_timeout);
                adaptive_timeout_backoff(&adaptive_timeout);
                rto = timespec_to_double(adaptive_timeout.rto);
                logger_log_trace(request->logger, "Adaptive timeout: RTO set to %lld.%.9ld seconds via exponential backoff.", adaptive_timeout.rto.tv_sec, adaptive_timeout.rto.tv_nsec);
            }
            clock_gettime(CLOCK_MONOTONIC, &start_time);
            for (uint16_t i = window_begin; is_in_range(i, window_begin, next_sequence_number - 1); i++) {
                uint16_t packet_index = (i - 1) % request->options.window_size;
                size_t offset = packet_index * data_packet_size;
                struct tftp_data_packet *packet = (struct tftp_data_packet *)((uint8_t *)data_packets + offset);
                size_t packet_size = (i == next_sequence_number - 1) ? last_packet_size : sizeof(*packet) + request->options.block_size;
                
                ssize_t ret = sendto(request->connection->sockfd,
                                     packet,
                                     packet_size,
                                     0,
                                     (struct sockaddr *)&request->connection->server_addr.sockaddr,
                                     request->connection->server_addr.socklen);
                if (ret == -1) {
                    logger_log_error(request->logger, "Failed to retransmit data packet for block %d. %s", i, strerror(errno));
                    goto fail;
                }
                if (ret < (ssize_t) packet_size) {
                    logger_log_warn(request->logger, "Could not send whole packet. The file transfer failed but the server will not be able to detect it.");
                    // TODO handle tsize option
                    goto fail;
                }
                logger_log_trace(request->logger, "Retransmitted DATA packet <block=%d, size=%zu bytes>.", i, packet_size - sizeof(*packet));
            }
            continue;
        }
        
        enum tftp_opcode opcode = tftp_get_opcode_unsafe(request->packet_recv_buffer);
        switch (opcode) {
            case TFTP_OPCODE_ACK:
                struct tftp_ack_packet *ack_packet = (struct tftp_ack_packet *) request->packet_recv_buffer;
                uint16_t block_number = ntohs(ack_packet->block_number);
                if (!is_in_range(block_number, window_begin, next_sequence_number - 1)) {
                    logger_log_trace(request->logger, "Received unexpected ACK <block=%d> not in window [%d, %d]. Ignoring packet.", block_number, window_begin, next_sequence_number - 1);
                    break;
                }
                logger_log_trace(request->logger, "Received ACK <block=%d>", block_number);
                window_begin = block_number + 1;
                if (window_begin != next_sequence_number) {
                    clock_gettime(CLOCK_MONOTONIC, &start_time);
                }
                transfer_completed = end_of_file && block_number == next_sequence_number - 1;
                retransmissions = 0;
                if (request->options.use_adaptive_timeout && is_in_range(block_number, adaptive_timeout.starting_block_number, next_sequence_number - 1)) {
                    adaptive_timeout_stop_timer(&adaptive_timeout);
                    rto = timespec_to_double(adaptive_timeout.rto);
                    logger_log_trace(request->logger, "Adaptive timeout: RTO set to %lld.%.9ld seconds.", adaptive_timeout.rto.tv_sec, adaptive_timeout.rto.tv_nsec);
                }
                break;
            case TFTP_OPCODE_ERROR:
                log_error_packet_received(request->logger, (struct tftp_error_packet *) request->packet_recv_buffer, bytes_received);
                goto fail;
            default:
                logger_log_error(request->logger, "Unexpected opcode %d", opcode);
                goto fail;
        }
    }
    free(data_packets);
    return true;
fail:
    free(data_packets);
    return false;
}

static bool handle_oack_packet(struct request request[static 1], ssize_t packet_size) {
    struct tftp_oack_packet *oack_packet = (struct tftp_oack_packet *) request->packet_recv_buffer;
    struct tftp_option ackd_options[TFTP_OPTION_TOTAL_OPTIONS] = {};
    char ackd_options_str[tftp_option_formatted_string_max_size];
    bool contains_unrequested_options = false;
    uint16_t block_size = request->options.block_size;
    uint16_t timeout = request->options.timeout_s;
    uint16_t window_size = request->options.window_size;
    tftp_parse_options(ackd_options, packet_size - sizeof *oack_packet, (char *) oack_packet->options_values);
    if (request->options.use_adaptive_timeout && request->request_type == REQUEST_GET) {
        request->options.use_adaptive_timeout = false;
        const char *option = (char *) oack_packet->options_values;
        const char *end_ptr = &((char *) oack_packet->options_values)[packet_size - sizeof *oack_packet - 1];
        while (option < end_ptr) {
            const char *val = &option[strlen(option) + 1];
            if (strcasecmp(option, tftp_option_recognized_string[TFTP_OPTION_TIMEOUT]) == 0
                && strcasecmp(val, "adaptive") == 0) {
                ackd_options[TFTP_OPTION_TIMEOUT] = (struct tftp_option) {
                    .is_active = true,
                    .value = "adaptive",
                };
                request->options.use_adaptive_timeout = true;
                break;
            }
            val = &option[strlen(option) + 1];
            option = &val[strlen(val) + 1];
        }
    }
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
                    if (request->options.use_adaptive_timeout) {
                        break;
                    }
                    timeout = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TSIZE:
                    request->options.tsize = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_WINDOWSIZE:
                    window_size = strtoul(ackd_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_READ_TYPE:
                    break;
                default:
                    unreachable();
            }
        }
    }
    if (contains_unrequested_options) {
        logger_log_error(request->logger, "Received OACK with unrequested options.");
        send_error(request, TFTP_ERROR_INVALID_OPTIONS, nullptr);
        return false;
    }
    if (block_size != request->options.block_size || window_size != request->options.window_size) {
        request->options.block_size = block_size;
        request->options.window_size = window_size;
        request->packet_recv_buffer_size = tftp_packet_buffer_size(request->options.block_size);
        uint8_t *new_buffer = realloc(request->packet_recv_buffer, request->packet_recv_buffer_size);
        if (new_buffer == nullptr) {
            logger_log_error(request->logger, "Failed to reallocate memory for the buffer. %s", strerror(errno));
            return false;
        }
        request->packet_recv_buffer = new_buffer;
    }
    if (!request->options.use_adaptive_timeout && timeout != request->options.timeout_s) {
        request->options.timeout_s = timeout;
        struct timeval new_timeout = {.tv_sec = request->options.timeout_s,};
        if (setsockopt(request->connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, &new_timeout, sizeof new_timeout) == -1) {
            logger_log_error(request->logger, "Could not set listener socket options for the server. %s", strerror(errno));
            return false;
        }
    }
    if (request->options.options[TFTP_OPTION_READ_TYPE].is_active && !ackd_options[TFTP_OPTION_READ_TYPE].is_active) {
        request->options.options[TFTP_OPTION_READ_TYPE].is_active = false;
    }
    return true;
}

static void handle_unexpected_peer(struct request request[static 1], struct server_sockaddr server_addr[static 1]) {
    struct server_sockaddr *expected = &request->connection->server_addr;
    struct server_sockaddr *actual = server_addr;
    char expected_addr_str[INET6_ADDRSTRLEN];
    uint16_t expected_port;
    bool expected_addr_string_resolved = (
        sockaddr_ntop((struct sockaddr *) &expected->sockaddr, expected_addr_str, &expected_port) != nullptr);
    if (!expected_addr_string_resolved) {
        logger_log_warn(request->logger, "Could not translate expected address to a string representation. %s", strerror(errno));
    }
    char actual_addr_str[INET6_ADDRSTRLEN];
    uint16_t actual_port;
    bool actual_addr_string_resolved = (
        sockaddr_ntop((struct sockaddr *) &actual->sockaddr, actual_addr_str, &actual_port) != nullptr);
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

static bool send_rrq(struct request request[static 1]) {
    struct tftp_rrq_packet rrq_packet;
    ssize_t rrq_packet_size = tftp_rrq_packet_init(&rrq_packet,
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

static bool send_wrq(struct request request[static 1]) {
    struct tftp_wrq_packet wrq_packet;
    ssize_t wrq_packet_size = tftp_wrq_packet_init(&wrq_packet,
                                                   strlen(request->details.put.filename) + 1,
                                                   request->details.put.filename,
                                                   request->details.put.mode,
                                                   request->options.options);
    if (wrq_packet_size < 0) {
        logger_log_error(request->logger, "Failed to create WRQ packet.");
        return false;
    }
    if (!send_wrq_packet(request, &wrq_packet, wrq_packet_size)) {
        return false;
    }
    request->last_packet_sent.wrq_packet = wrq_packet;
    request->last_packet_sent_size = wrq_packet_size;
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
        case TFTP_OPCODE_WRQ:
            ret = send_wrq_packet(request, &request->last_packet_sent.wrq_packet, request->last_packet_sent_size);
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

static bool send_rrq_packet(struct request request[static 1],
                            const struct tftp_rrq_packet rrq_packet[static 1],
                            size_t rrq_packet_size) {
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
    logger_log_trace(request->logger, "Sent RRQ <file=%s, mode=%s%s%s>",
                     request->details.get.filename,
                     request->details.get.mode_str,
                     request->use_options ? ", options=" : "",
                     request->use_options ? request->options.options_str : "");
    return true;
}

static bool send_wrq_packet(struct request request[static 1],
                            const struct tftp_wrq_packet wrq_packet[static 1],
                            size_t wrq_packet_size) {
    ssize_t bytes_sent = sendto(request->connection->sockfd,
                                wrq_packet,
                                wrq_packet_size,
                                0,
                                (struct sockaddr *) &request->connection->server_addr.sockaddr,
                                request->connection->server_addr.socklen);
    if (bytes_sent < 0) {
        logger_log_error(request->logger, "Failed to send WRQ packet. %s", strerror(errno));
        return false;
    }
    logger_log_trace(request->logger, "Sent WRQ <file=%s, mode=%s%s%s>",
                     request->details.put.filename,
                     request->details.put.mode_str,
                     request->use_options ? ", options=" : "",
                     request->use_options ? request->options.options_str : "");
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

static bool send_error_packet(struct request request[static 1],
                              const struct tftp_error_packet *error_packet,
                              size_t error_packet_size) {
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

static ssize_t receive_packet(struct request request[static 1], struct server_sockaddr server_addr[static 1]) {
    auto total_attempts = 1 + request->retries;
    uint8_t retransmissions = 0;
    ssize_t bytes_received = 0;
    while (retransmissions < total_attempts) {
        bytes_received = recvfrom(request->connection->sockfd,
                                  request->packet_recv_buffer,
                                  request->packet_recv_buffer_size,
                                  0,
                                  (struct sockaddr *) &server_addr->sockaddr,
                                  &server_addr->socklen);
        if (bytes_received != -1) {
            break;
        }
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            logger_log_error(request->logger, "Failed to receive data. %s", strerror(errno));
            return RECEIVE_PACKET_ERROR;
        }
        if (retransmissions == request->retries) {
            logger_log_error(request->logger, "Transfer timed out after %d of %d retransmissions.", retransmissions, request->retries);
            return RECEIVE_PACKET_TIMEOUT;
        }
        logger_log_warn(request->logger, "Receive timed out. Retransmitting last packet.");
        if (!retransmit_last_packet_sent(request)) {
            return RECEIVE_PACKET_ERROR;
        }
        retransmissions++;
    }
    return bytes_received;
}

static inline void log_error_packet_received(struct logger logger[static 1],
                                             struct tftp_error_packet *error_packet,
                                             size_t packet_size) {
    if (packet_size < sizeof *error_packet + 1) {
        logger_log_error(logger, "Received error packet is too small.");
        return;
    }
    if (error_packet == nullptr) {
        logger_log_fatal(logger, "Pointer to the received error packet is a null pointer.");
        return;
    }
    uint16_t error_code = ntohs(error_packet->error_code);
    const char *error_message = (const char *) error_packet->error_message;
    size_t message_size = packet_size - sizeof *error_packet;
    if (strnlen(error_message, message_size) == message_size) {
        error_message = "Error message is not null-terminated.";
    }
    logger_log_error(logger, "Received error %d: %s", error_code, error_message);
}

static inline size_t tftp_packet_buffer_size(uint16_t required_block_size) {
    //constexpr size_t min_buffer_size = tftp_oack_packet_max_size;
    constexpr size_t min_buffer_size = 516; // data_packet size + default_block_size since it is handled poorly
    size_t required_buffer_size = sizeof(struct tftp_data_packet) + required_block_size;
    return required_buffer_size < min_buffer_size ? min_buffer_size : required_buffer_size;
}
