#include <signal.h>
#include <stdlib.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include "acceptor.h"
#include "cmdline.h"
#include "dispatcher.h"
#include "tftp_service.h"

struct tftp_service_factory {
    struct acceptor* acceptor;
    struct dispatcher* dispatcher;
    struct tftp_service_parameters tftp_service_parameters;
};

static void tftp_on_request(evutil_socket_t peer_acceptor, short event, void *arg);
static inline void set_log_level(int verbose_level);

extern int main(int argc, char *argv[argc + 1]) {
	cmdline_args_t args;
    try(parse_cmdline_args(argc, (const char**)argv, &args), 1, argparser_fail);
    struct acceptor acceptor = {};
    struct dispatcher dispatcher = {};
	dispatcher_event_t dispatcher_sigterm_event;
	dispatcher_event_t dispatcher_sigint_event;
    dispatcher_event_t accept_event;
    struct tftp_service_factory tftp_service_factory = {
        .acceptor = &acceptor,
        .dispatcher = &dispatcher,
        .tftp_service_parameters = {
            .directory = args.directory,
            .window_size = args.window_size,
            .timeout_duration = args.timeout_duration,
            .loss_probability = args.loss_probability
        }
    };
    set_log_level(args.verbose_level);
	try(dispatcher_init(&dispatcher, args.thread_count), -1, dispatcher_init_fail);
	try(acceptor_init(&acceptor, args.listen_port), 1, acceptor_init_fail);
    try((accept_event = event_new(dispatcher.event_base, acceptor.peer_acceptor, EV_READ | EV_PERSIST, tftp_on_request, &tftp_service_factory)), nullptr, event_new_fail);
    try(event_add(accept_event, nullptr), -1, event_add_fail);
	try(dispatcher_sigterm_event = dispatcher_stop_on_signal_event_add(&dispatcher, SIGTERM), nullptr, dispatcher_register_signal_handler_fail);
	try(dispatcher_sigint_event = dispatcher_stop_on_signal_event_add(&dispatcher, SIGINT), nullptr, dispatcher_register_signal_handler_fail);
	try(dispatcher_dispatch(&dispatcher), -1, dispatcher_dispatch_fail);
	cmn_logger_log_info("All events have been terminated.");
    try(dispatcher_event_remove(accept_event), -1, remove_event_fail);
	try(acceptor_destroy(&acceptor), 1, acceptor_destroy_fail);
	try(dispatcher_event_remove(dispatcher_sigterm_event), -1, remove_event_fail);
	try(dispatcher_event_remove(dispatcher_sigint_event), -1, remove_event_fail);
	dispatcher_destroy(&dispatcher);
	return EXIT_SUCCESS;
argparser_fail:
	cmn_logger_log_error("Can't parse CLI arguments.");
	return EXIT_FAILURE;
dispatcher_init_fail:
	cmn_logger_log_error("Can't initialize dispatcher.");
	return EXIT_FAILURE;
dispatcher_register_signal_handler_fail:
	return EXIT_FAILURE;
event_new_fail:
    cmn_logger_log_error("Can't create an accept event.");
    return EXIT_FAILURE;
event_add_fail:
    cmn_logger_log_error("Couldn't set accept event status to pending.");
    return EXIT_FAILURE;
acceptor_init_fail:
	cmn_logger_log_error("Can't initialize acceptor.");
	return EXIT_FAILURE;
dispatcher_dispatch_fail:
	cmn_logger_log_error("Event loop aborted for an unhandled backend error.");
	return EXIT_FAILURE;
remove_event_fail:
	cmn_logger_log_error("Error encountered when removing a dispatcher event.");
	return EXIT_FAILURE;
acceptor_destroy_fail:
	cmn_logger_log_error("Error encountered when destroying acceptor.");
	return EXIT_FAILURE;
}

enum tftp_mode {
    OCTET,
    NETASCII,
    INVALID_MODE
};

enum tftp_opcode : uint16_t {
    RRQ = 0x01,
    WRQ = 0x02,
    DATA = 0x03,
    ACK = 0x04,
    ERROR = 0x05,
};

enum tftp_error_code : uint16_t {
    NOT_DEFINED = 0x00,
    FILE_NOT_FOUND = 0x01,
    ACCESS_VIOLATION = 0x02,
    DISK_FULL = 0x03,
    ILLEGAL_OPERATION = 0x04,
    UNKNOWN_TRANSFER_ID = 0x05,
    FILE_ALREADY_EXISTS = 0x06,
    NO_SUCH_USER = 0x07,
    INVALID_OPTIONS = 0x08,
};

struct [[gnu::packed]] tftp_packet {
    enum tftp_opcode opcode;
    uint8_t padding[514];
};

struct [[gnu::packed]] tftp_packet_rrq {
    enum tftp_opcode opcode;
    uint8_t details[510];
};

struct [[gnu::packed]] tftp_packet_wrq {
    enum tftp_opcode opcode;
    uint8_t details[510];
};

struct [[gnu::packed]] tftp_packet_data {
    enum tftp_opcode opcode;
    uint16_t block_number;
    uint8_t data[512];
};

struct [[gnu::packed]] tftp_packet_ack {
    enum tftp_opcode opcode;
    uint16_t block_number;
};

struct [[gnu::packed]] tftp_packet_error {
    enum tftp_opcode opcode;
    enum tftp_error_code error_code;
    uint8_t error_message[512];
};

enum tftp_mode tftp_mode_from_string(const char *mode) {
    if (evutil_ascii_strcasecmp(mode, "octet") == 0) {
        return OCTET;
    }
    if (evutil_ascii_strcasecmp(mode, "netascii") == 0) {
        return NETASCII;
    }
    return INVALID_MODE;
}

struct tftp_request {
    struct dispatcher* dispatcher;
    struct tftp_service_parameters tftp_service_parameters;
    evutil_socket_t socket;
    struct tftp_packet *packet;
};

void tftp_rrq_handler(struct tftp_request* request);
void tftp_wrq_handler(struct tftp_request* request);

static void (*tftp_request_handlers[])(struct tftp_request* request) = {
    [RRQ] = tftp_rrq_handler,
    [WRQ] = nullptr,
};

struct tftp_file_transfer_state {
    evutil_socket_t socket;
    FILE* file;
    enum tftp_mode mode;
    size_t block_number;
    int8_t retries;
    dispatcher_event_t response_or_timeout_event;
    struct timeval timeout;
    struct tftp_packet_data last_packet;
    size_t last_packet_data_size;
};

static void tftp_on_response_or_timeout(evutil_socket_t socket, short event, void *arg);
void tftp_send_next_file_chunk(struct tftp_file_transfer_state *state);

void tftp_rrq_handler(struct tftp_request* request) {
    struct tftp_packet_rrq *packet = (struct tftp_packet_rrq *) request->packet;
    const char *filename = (const char *) packet->details;
    const char *mode_string = (const char *) packet->details + strlen(filename) + 1;
    cmn_logger_log_info("Received RRQ for file %s in mode %s.", filename, mode_string);
    enum tftp_mode mode = tftp_mode_from_string(mode_string);
    if (mode == INVALID_MODE) {
        cmn_logger_log_info("Invalid mode %s, closing connection.", mode_string);
        return;
    }
    FILE* file = fopen(filename, "rb");
    if (file == nullptr) {
        // TODO: check why the file couldn't be opened and send the appropriate error message
        cmn_logger_log_error("Couldn't open file %s.", filename);
        struct tftp_packet_error error = {
            .opcode = ERROR,
            .error_code = FILE_NOT_FOUND,
            .error_message = "File not found."
        };
        send(request->socket, &error, sizeof error, 0);
        evutil_closesocket(request->socket);
        return;
    }
    struct tftp_file_transfer_state *state = malloc(sizeof *state);
    *state = (struct tftp_file_transfer_state){
        .socket = request->socket,
        .file = file,
        .mode = mode,
        .block_number = 0,
        .retries = 0,
        .response_or_timeout_event = nullptr,
        .timeout = {
            .tv_sec = request->tftp_service_parameters.timeout_duration,
            .tv_usec = 0
        },
        .last_packet = {
            .opcode = htons(DATA),
        },
        .last_packet_data_size = 0,
    };
    try((state->response_or_timeout_event = event_new(request->dispatcher->event_base,
                                                     request->socket,
                                                     EV_READ | EV_PERSIST,
                                                     tftp_on_response_or_timeout,
                                                     state)), nullptr, event_new_fail);
    try(event_add(state->response_or_timeout_event, &state->timeout), -1, event_add_fail);
    tftp_send_next_file_chunk(state);
    return;
event_new_fail:
    cmn_logger_log_error("Couldn't create event.");
    return;
event_add_fail:
    cmn_logger_log_error("Couldn't set response event status to pending.");
}

void tftp_send_next_file_chunk(struct tftp_file_transfer_state *state) {
    ++state->block_number;
    state->last_packet.block_number = htons(state->block_number);
    if (!feof(state->file)) {
        state->last_packet_data_size = fread(state->last_packet.data, 1, sizeof state->last_packet.data, state->file);
    }
    if (ferror(state->file)) {
        // TODO: check error and send the appropriate error message
        cmn_logger_log_error("Error while reading file.");
        return;
    }
    if (feof(state->file)) {
        cmn_logger_log_info("End of file reached. Sending packet with data field of size %zu to signal end of transfer.", state->last_packet_data_size);
        fclose(state->file);
        state->file = nullptr;
    }
    try(send(state->socket, &state->last_packet, 4 + state->last_packet_data_size, 0), -1, send_fail);
    cmn_logger_log_trace("Sent block %d.", state->block_number);
    return;
send_fail:
    cmn_logger_log_error("Couldn't send data packet.");
}

static void tftp_on_request(evutil_socket_t, short, void *arg) {
    struct tftp_service_factory *tftp_service_factory = arg;
    evutil_socket_t socket;
    struct acceptor_packet accepted_packet;
    socket = acceptor_accept(tftp_service_factory->acceptor, &accepted_packet);
    if (socket == EVUTIL_INVALID_SOCKET) {
        return;
    }
    struct tftp_packet tftp_packet = {
        .opcode = ntohs(*(uint16_t *)accepted_packet.data),
    };
    memcpy(tftp_packet.padding, accepted_packet.data + sizeof tftp_packet.opcode, 510);
    if (tftp_packet.opcode == RRQ || tftp_packet.opcode == WRQ) {
        struct tftp_request request = {
            .dispatcher = tftp_service_factory->dispatcher,
            .tftp_service_parameters = tftp_service_factory->tftp_service_parameters,
            .socket = socket,
            .packet = &tftp_packet,
        };
        tftp_request_handlers[tftp_packet.opcode](&request);
    }
    else {
        cmn_logger_log_info("Acceptor: Received ill formed connection request. Closing client connection.");
        struct tftp_packet_error error = {
            .opcode = ERROR,
            .error_code = ILLEGAL_OPERATION,
            .error_message = "Connections can be required sending a request to read or write a file."
        };
        send(socket, &error, sizeof error, 0);
        try(evutil_closesocket(socket), -1, fail);
    }
fail:

}

static void tftp_on_timeout(struct tftp_file_transfer_state *state) {
    if (state->retries < 10) {
        cmn_logger_log_info("Timeout reached. Retrying to send block %d.", state->block_number);
        send(state->socket, &state->last_packet, 4 + state->last_packet_data_size, 0);
        state->retries++;
        return;
    }
    cmn_logger_log_info("Timeout reached. Closing connection.");
    struct tftp_packet_error error = {
        .opcode = ERROR,
        .error_code = NOT_DEFINED,
        .error_message = "Timeout reached."
    };
    send(state->socket, &error, sizeof error, 0);
    evutil_closesocket(state->socket);
    event_free(state->response_or_timeout_event);
    free(state);
}

static void tftp_on_response_or_timeout(evutil_socket_t socket, short event, void *arg) {
    struct tftp_file_transfer_state *state = arg;
    if (event == EV_TIMEOUT) {
        tftp_on_timeout(state);
        return;
    }
    struct tftp_packet packet = {};
    ssize_t received_bytes = recv(socket, &packet, sizeof packet, 0);
    if (received_bytes == -1) {
        goto receive_fail;
    }
    switch (ntohs(packet.opcode)) {
        case ACK: {
            struct tftp_packet_ack *ack = (struct tftp_packet_ack *) &packet;
            if (received_bytes != 4) {
                cmn_logger_log_info("Received ACK packet with invalid size %zd. Closing connection.", received_bytes);
                return;
            }
            if (ntohs(ack->block_number) != state->block_number) {
                cmn_logger_log_info("Received ACK for block %d. Expected ACK for block %d.", ntohs(ack->block_number), state->block_number);
                return;
            }
            cmn_logger_log_info("Received ACK for block %d.", state->block_number);
            if (feof(state->file)) {
                cmn_logger_log_info("File sent successfully. Closing connection.");
                fclose(state->file);
                evutil_closesocket(state->socket);
                event_free(state->response_or_timeout_event);
                free(state);
                return;
            }
            tftp_send_next_file_chunk(state);
            return;
        }
        case ERROR: {
            struct tftp_packet_error *error = (struct tftp_packet_error *) &packet;
            cmn_logger_log_info("Received error packet with code %d and message %s.", ntohs(error->error_code), error->error_message);
            evutil_closesocket(state->socket);
            return;
        }
        default:
            cmn_logger_log_info("Received unexpected packet with opcode %d.", ntohs(packet.opcode));
            return;
    }
receive_fail:
    cmn_logger_log_info("Error while receiving packet.");
}

static inline void set_log_level(int verbose_level) {
	switch (verbose_level) {
	case 1:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_WARN);
		break;
	case 2:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_INFO);
		break;
	case 3:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_DEBUG);
		break;
	default:
		cmn_logger_set_default_level(CMN_LOGGER_LOG_LEVEL_ALL);
	}
}
