#include "tftp_client.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <inet_utils.h>
#include <tftp.h>
#include <stdlib.h>

struct transfer_stats {
    bool transfer_complete;
    bool first_receive;
    uint8_t retransmits;
    uint16_t expected_next_block;
    uint16_t last_block_received;
    size_t buffer_size;
    uint8_t *buffer;
};

static bool listener_init(struct tftp_client client[static 1], const char *host, const char *service);
static void options_init(struct tftp_client client[static 1]);

static bool get(struct tftp_client client[static 1]);
static bool put(struct tftp_client client[static 1]);
static bool list(struct tftp_client client[static 1]);

static bool send_read_request(struct tftp_client client[static 1]);
static bool receive_file(struct tftp_client client[static 1], FILE file[static 1]);
static bool write_tmp_to_file(struct tftp_client client[static 1], FILE tmp[static 1]);

static bool send_rrq_packet(struct tftp_client client[static 1]);
static bool send_ack_packet(struct tftp_client client[static 1], uint16_t received_block_number);

static ssize_t receive_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], bool unexpected_peer[static 1]);
static bool handle_packet(struct tftp_client client[static 1], FILE file[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);

static bool handle_error_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);
static bool handle_oack_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size);
static bool handle_data_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size, FILE file[static 1]);

static inline size_t get_buffer_size(struct tftp_client client[static 1]);

bool tftp_client_init(struct tftp_client client[static 1], struct tftp_client_arguments args, struct logger logger[static 1]) {
    *client = (struct tftp_client) {
        .client_stats_callback = args.client_stats_callback,
        .sockfd = -1,
        .sock_addr_len = sizeof client->sock_addr_storage,
        .logger = logger,
        .command = args.command,
        .mode = args.mode,
        .mode_str = tftp_mode_to_string(args.mode),
        .filename = args.filename,
        .retries = args.retries,
        .timeout_s = args.timeout_s,
        .block_size = args.block_size,
        .window_size = args.window_size,
        .use_tsize = args.use_tsize,
        .adaptive_timeout = args.adaptive_timeout,
        .loss_probability = args.loss_probability,
    };
    options_init(client);
    if (!listener_init(client, args.host, args.port)) {
        return false;
    }
    return true;
}

bool tftp_client_start(struct tftp_client client[static 1]) {
    switch (client->command) {
        case CLIENT_COMMAND_LIST:
            return list(client);
        case CLIENT_COMMAND_GET:
            return get(client);
        case CLIENT_COMMAND_PUT:
            return put(client);
    }
    return true;
}

void tftp_client_destroy(struct tftp_client client[static 1]) {
    if (close(client->sockfd)) {
        logger_log_error(client->logger, "close: %s", strerror(errno));
    }
}

static bool listener_init(struct tftp_client client[static 1], const char *host, const char *service) {
    struct addrinfo *addrinfo_list = nullptr;
    struct timeval timeout = {.tv_sec = client->timeout_s,};
    static const struct addrinfo hints = {
        .ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED,
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_DGRAM,
        .ai_addr = nullptr,
        .ai_canonname = nullptr,
        .ai_next = nullptr
    };
    int gai_ret = getaddrinfo(host, service, &hints, &addrinfo_list);
    if (gai_ret) {
        logger_log_error(client->logger, "Could not translate the host and service required to a valid internet address. %s", gai_strerror(gai_ret));
        if (addrinfo_list != nullptr) {
            freeaddrinfo(addrinfo_list);
        }
        return false;
    }
    struct addrinfo *addrinfo = addrinfo_list;
    client->sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (inet_ntop_address(addrinfo->ai_addr, client->server_addr_str, &client->server_port) == nullptr) {
        logger_log_error(client->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        goto fail;
    }
    if (client->sockfd == -1) {
        logger_log_error(client->logger, "Could not create a socket for the server. %s", strerror(errno));
        goto fail;
    }
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {true}, sizeof(int)) == -1 ||
        setsockopt(client->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {false}, sizeof(int)) == -1 ||
        setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) == -1) {
        logger_log_error(client->logger, "Could not set listener socket options for the server. %s", strerror(errno));
        goto fail2;
    }
    memcpy(&client->server_addr_storage, addrinfo->ai_addr, addrinfo->ai_addrlen);
    client->server_addr_len = addrinfo->ai_addrlen;
    freeaddrinfo(addrinfo_list);
    return true;
fail2:
    close(client->sockfd);
fail:
    freeaddrinfo(addrinfo_list);
    return false;
}

static void options_init(struct tftp_client client[static 1]) {
    if (client->timeout_s == 0) {
        client->timeout_s = 2;
        client->is_timeout_s_default = true;
    }
    else {
        sprintf((char *) client->timeout_s_str, "%hhu", client->timeout_s);
    }
    if (client->block_size == 0) {
        client->block_size = tftp_default_blksize;
        client->is_block_size_default = true;
    }
    else {
        sprintf((char *) client->block_size_str, "%hu", client->block_size);
    }
    if (client->window_size == 0) {
        client->window_size = tftp_default_window_size;
        client->is_window_size_default = true;
    }
    else {
        sprintf((char *) client->window_size_str, "%hu", client->window_size);
    }
    sprintf((char *) client->tsize_str, "0");
    
    memcpy(client->options,
           &(struct tftp_option[TFTP_OPTION_TOTAL_OPTIONS]) {
               [TFTP_OPTION_BLKSIZE] = {.is_active = !client->is_block_size_default, .value = (const char *) client->block_size_str},
               [TFTP_OPTION_TIMEOUT] = {.is_active = !client->is_timeout_s_default, .value = (const char *) client->timeout_s_str},
               [TFTP_OPTION_TSIZE] = {.is_active = client->use_tsize, .value = (const char *) client->tsize_str},
               [TFTP_OPTION_WINDOWSIZE] = {.is_active = !client->is_window_size_default, . value = (const char *) client->window_size_str},
           },
           sizeof client->options);
    for (size_t i = 0; i < TFTP_OPTION_TOTAL_OPTIONS; i++) {
        if (client->options[i].is_active) {
            client->use_options = true;
            break;
        }
    }
    if (client->use_options) {
        tftp_format_options(client->options, client->options_str);
    }
}

static bool get(struct tftp_client client[static 1]) {
    FILE* tmp = tmpfile();
    if (tmp == nullptr) {
        logger_log_error(client->logger, "Could not create temporary file. %s", strerror(errno));
        return false;
    }
    logger_log_info(client->logger, "Getting file %s from %s:%hu [%s]", client->filename, client->server_addr_str, client->server_port, client->mode_str);
    tftp_client_stats_init(&client->stats, client->logger);
    if (!send_read_request(client)) {
        fclose(tmp);
        return false;
    }
    if (!receive_file(client, tmp)) {
        fclose(tmp);
        return false;
    }
    client->client_stats_callback(&client->stats);
    if (client->use_tsize && client->stats.file_bytes_received != client->tsize) {
        logger_log_error(client->logger, "Received file size does not match the expected size.");
        fclose(tmp);
        return false;
    }
    if (!write_tmp_to_file(client, tmp)) {
        fclose(tmp);
        return false;
    }
    if (fclose(tmp) == EOF) {
        logger_log_error(client->logger, "Failed to close the temporary file. %s", strerror(errno));
        return false;
    }
    return true;
}

static bool put(struct tftp_client client[static 1]) {
    return false;
}

static bool list(struct tftp_client client[static 1]) {
    return false;
}

static bool send_read_request(struct tftp_client client[static 1]) {
    client->rrq_packet_size = tftp_rrq_packet_init(&client->rrq_packet,
                                                   strlen(client->filename) + 1,
                                                   client->filename,
                                                   client->mode,
                                                   client->options);
    if (client->rrq_packet_size < 0) {
        logger_log_error(client->logger, "Failed to create RRQ packet");
        return false;
    }
    if (!send_rrq_packet(client)) {
        return false;
    }
    if (getsockname(client->sockfd, (struct sockaddr *) &client->sock_addr_storage, &client->sock_addr_len) == -1) {
        logger_log_error(client->logger, "Failed to get the socket name. %s", strerror(errno));
        return false;
    }
    if (inet_ntop_address((struct sockaddr *) &client->sock_addr_storage, client->sock_addr_str, &client->sock_port) == nullptr) {
        logger_log_error(client->logger, "Could not translate binding address to a string representation. %s", strerror(errno));
        return false;
    }
    logger_log_debug(client->logger, "Bound to %s:%hu", client->sock_addr_str, client->sock_port);
    return true;
}

static bool receive_file(struct tftp_client client[static 1], FILE file[static 1]) {
    struct transfer_stats stats = {
        .first_receive = true,
        .expected_next_block = 1,
        .buffer_size = get_buffer_size(client),
        .buffer = malloc(stats.buffer_size),
    };
    if (stats.buffer == nullptr) {
        logger_log_error(client->logger, "Failed to allocate memory for the buffer. %s", strerror(errno));
        return false;
    }
    while (!stats.transfer_complete) {
        bool unexpected_peer = false;
        memset(stats.buffer, 0, stats.buffer_size);
        ssize_t bytes_received = receive_packet(client, &stats, &unexpected_peer);
        stats.first_receive = false;
        if (bytes_received == -1) {
            free(stats.buffer);
            return false;
        }
        if (unexpected_peer) {
            continue;
        }
        if (!handle_packet(client, file, &stats, bytes_received)) {
            free(stats.buffer);
            return false;
        }
    }
    free(stats.buffer);
    return true;
}

static bool write_tmp_to_file(struct tftp_client client[static 1], FILE tmp[static 1]) {
    FILE *file = fopen(client->filename, "wb");
    if (file == nullptr) {
        logger_log_error(client->logger, "Could not open file %s. %s", client->filename, strerror(errno));
        return false;
    }
    rewind(tmp);
    int c;
    while ((c = fgetc(tmp)) != EOF) {
        if (ferror(tmp)) {
            logger_log_error(client->logger, "Failed to read from the temporary file. %s", strerror(errno));
            fclose(file);
            return false;
        }
        fputc(c, file);
        if (ferror(file)) {
            logger_log_error(client->logger, "Failed to write to the file. %s", strerror(errno));
            fclose(file);
            return false;
        }
    }
    if (fclose(file) == EOF) {
        logger_log_error(client->logger, "Failed to close the file. %s", strerror(errno));
        return false;
    }
    return true;
}

static bool send_rrq_packet(struct tftp_client client[static 1]) {
    if (sendto(client->sockfd, &client->rrq_packet, client->rrq_packet_size, 0, (struct sockaddr *) &client->server_addr_storage, client->server_addr_len) < 0) {
        logger_log_error(client->logger, "Failed to send RRQ packet. %s", strerror(errno));
        return false;
    }
    logger_log_trace(client->logger, "Sent RRQ <file=%s, mode=%s%s%s>", client->filename, client->mode_str, client->use_options ? ", options=" : "", client->use_options ? client->options_str : "");
    return true;
}

static bool send_ack_packet(struct tftp_client client[static 1], uint16_t received_block_number) {
    struct tftp_ack_packet ack_packet;
    tftp_ack_packet_init(&ack_packet, received_block_number);
    if (sendto(client->sockfd, &ack_packet, sizeof ack_packet, 0, (struct sockaddr *) &client->server_addr_storage, client->server_addr_len) < 0) {
        logger_log_error(client->logger, "Failed to send ACK packet. %s", strerror(errno));
        return false;
    }
    logger_log_trace(client->logger, "Sent ACK <block=%d>", received_block_number);
    return true;
}

static ssize_t receive_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], bool unexpected_peer[static 1]) {
    ssize_t bytes_received;
    struct sockaddr_storage addr_storage = {};
    socklen_t addr_len = sizeof addr_storage;
    struct sockaddr *addr_ptr = stats->first_receive ?
                                (struct sockaddr *) &client->server_addr_storage :
                                (struct sockaddr *) &addr_storage;
    socklen_t *addr_len_ptr = stats->first_receive ? &client->server_addr_len : &addr_len;
    while (++stats->retransmits < client->retries) {
        bytes_received = recvfrom(client->sockfd, stats->buffer, stats->buffer_size, 0, addr_ptr, addr_len_ptr);
        if (bytes_received != -1) {
            break;
        }
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            bool result = stats->first_receive ? send_rrq_packet(client) : send_ack_packet(client, stats->last_block_received);
            if (!result) {
                return -1;
            }
            continue;
        }
        logger_log_error(client->logger, "Failed to receive data. %s", strerror(errno));
        return -1;
    }
    if (stats->retransmits == client->retries) {
        logger_log_error(client->logger, "Transfer timed out.", strerror(errno));
        return -1;
    }
    stats->retransmits = 0;
    if (!stats->first_receive) {
        if (addr_len != client->server_addr_len || memcmp(&client->server_addr_storage, &addr_storage, addr_len) != 0) {
            char addr_str[INET6_ADDRSTRLEN];
            uint16_t port;
            inet_ntop_address((struct sockaddr *) &addr_storage, addr_str, &port);
            logger_log_warn(client->logger, "Unexpected peer: '%s:%d', expected peer: '%s:%d'. Ignoring packet.", addr_str, port, client->server_addr_str, client->server_port);
            *unexpected_peer = true;
        }
    }
    return bytes_received;
}

static bool handle_packet(struct tftp_client client[static 1], FILE file[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    enum tftp_opcode opcode = ntohs(*(uint16_t *) stats->buffer);
    switch (opcode) {
        case TFTP_OPCODE_OACK:
            return handle_oack_packet(client, stats, packet_size);
        case TFTP_OPCODE_DATA:
            return handle_data_packet(client, stats, packet_size, file);
        case TFTP_OPCODE_ERROR:
            return handle_error_packet(client, stats, packet_size);
        default:
            logger_log_error(client->logger, "Unexpected opcode %d", opcode);
            return false;
    }
    return true;
}

static bool handle_error_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    struct tftp_error_packet *error_packet = (struct tftp_error_packet *) stats->buffer;
    uint16_t error_code = ntohs(error_packet->error_code);
    const char *error_message = (const char *) error_packet->error_message;
    logger_log_error(client->logger, "Received error %d: %s", error_code, error_message);
    if (stats->last_block_received == 0 && client->use_options) {
        client->server_maybe_do_not_support_options = true;
    }
    return false;
}

static bool handle_oack_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size) {
    struct tftp_oack_packet *oack_packet = (struct tftp_oack_packet *) stats->buffer;
    struct tftp_option ackd_options[TFTP_OPTION_TOTAL_OPTIONS] = {};
    char ackd_options_str[tftp_option_formatted_string_max_size];
    bool contains_unrequested_options = false;
    uint16_t block_size = client->block_size;
    uint16_t timeout = client->timeout_s;
    uint16_t window_size = client->window_size;
    tftp_parse_options(ackd_options, packet_size - sizeof *oack_packet, (char *) oack_packet->options_values);
    tftp_format_options(ackd_options, ackd_options_str);
    logger_log_trace(client->logger, "Received OACK %s", ackd_options_str);
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (ackd_options[o].is_active) {
            if (!client->options[o].is_active) {
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
                    client->tsize = strtoul(ackd_options[o].value, nullptr, 10);
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
        logger_log_error(client->logger, "Received OACK with unrequested options.");
        const struct tftp_error_packet_info *error_info = &tftp_error_packet_info[TFTP_ERROR_INVALID_OPTIONS];
        if (sendto(client->sockfd, error_info->packet, error_info->size, 0, (struct sockaddr *) &client->server_addr_storage, client->server_addr_len) < 0) {
            logger_log_error(client->logger, "Failed to send ERROR packet. %s", strerror(errno));
        }
        return false;
    }
    if (block_size != client->block_size || window_size != client->window_size) {
        client->block_size = block_size;
        client->window_size = window_size;
        stats->buffer_size = get_buffer_size(client);
        uint8_t *new_buffer = realloc(stats->buffer, stats->buffer_size);
        if (new_buffer == nullptr) {
            logger_log_error(client->logger, "Failed to reallocate memory for the buffer. %s", strerror(errno));
            return false;
        }
        stats->buffer = new_buffer;
    }
    if (timeout != client->timeout_s) {
        client->timeout_s = timeout;
        struct timeval new_timeout = {.tv_sec = client->timeout_s,};
        if (setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &new_timeout, sizeof new_timeout) == -1) {
            logger_log_error(client->logger, "Could not set listener socket options for the server. %s", strerror(errno));
            return false;
        }
    }
    if (!send_ack_packet(client, stats->last_block_received)) {
        return false;
    }
    return true;
}

static bool handle_data_packet(struct tftp_client client[static 1], struct transfer_stats stats[static 1], ssize_t packet_size, FILE file[static 1]) {
    struct tftp_data_packet *data_packet = (struct tftp_data_packet *) stats->buffer;
    stats->last_block_received = ntohs(data_packet->block_number);
    if (stats->last_block_received == stats->expected_next_block) {
        stats->expected_next_block++;
        size_t block_size = packet_size - sizeof *data_packet;
        fwrite(data_packet->data, 1, block_size, file);
        client->stats.file_bytes_received += block_size;
        logger_log_trace(client->logger, "Received DATA <block=%d, size=%zu bytes>", stats->last_block_received, block_size);
        if (!send_ack_packet(client, stats->last_block_received)) {
            return false;
        }
        if (block_size < client->block_size) {
            stats->transfer_complete = true;
        }
    }
    return true;
}

static inline size_t get_buffer_size(struct tftp_client client[static 1]) {
    static size_t min_buffer_size = sizeof(struct tftp_rrq_packet) - 4;
    size_t required_buffer_size = sizeof(struct tftp_data_packet) + client->block_size;
    return required_buffer_size < min_buffer_size ? min_buffer_size : required_buffer_size;
}
