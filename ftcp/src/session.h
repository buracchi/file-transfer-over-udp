#include <fts/fts.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/try.h>

enum fts_operation { NONE, LIST, GET, PUT };

struct fts_session {
	fts_t fts;
	char *url;
	cmn_socket2_t sock;
	struct {
		enum fts_operation last_operation;
		bool last_operation_waiting;
		char const * last_operation_file_name_arg;
		size_t last_operation_file_len_arg;
	} state;
};

extern fts_clinet_connection_t fts_client_reset_connection(fts_clinet_connection_t connection);
