#pragma once

#include <buracchi/common/networking/socket2.h>
#include <fts/fts.h>

#define FTS_CLIENT_DEFAULT_PORT 1234

enum fts_client_connection_state {
	FTS_CLIENT_STATE_IDLE,
	FTS_CLIENT_STATE_REQ_STARTED,
	FTS_CLIENT_STATE_REQ_SENT
};

enum fts_client_operation {
	FTS_CLIENT_OP_NONE,
	FTS_CLIENT_OP_LIST,
	FTS_CLIENT_OP_GET,
	FTS_CLIENT_OP_PUT
};

struct fts_client_connection_state_info {
	enum fts_client_connection_state state;
	enum fts_client_operation last_operation;
	bool last_operation_waiting;
	char const *last_operation_file_name_arg;
	size_t last_operation_file_len_arg;
};

struct fts_client_connection {
	fts_t fts;
	char *host;
	int port;
	float timeout;
	cmn_socket2_t sock;
	struct fts_client_connection_state_info state_info;
	enum cmn_logger_log_level log_level;
};

extern fts_client_connection_t fts_client_connection_init(fts_t fts, char const* host, int port, float timeout);

extern void fts_client_connection_destroy(fts_client_connection_t connection);

/**
 * @brief Close the connection to the FTCP server.
 */
extern void fts_client_connection_close(fts_client_connection_t connection);
