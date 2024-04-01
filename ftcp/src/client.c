#include "client.h"

#include <regex.h>
#include <string.h>

#include <buracchi/common/networking/nproto/nproto_service_ipv4.h>
#include <buracchi/common/networking/tproto/tproto_service_tcp.h>
#include <buracchi/common/utilities/strto.h>
#include <buracchi/common/utilities/try.h>

static int get_hostport(char const *host, int port, char **const connection_host, int *const connection_port);
static int validate_host(char const *host);

extern fts_client_connection_t fts_client_connection_init(fts_t fts, char const *host, int port, float timeout) {
	fts_client_connection_t connection;
	try(connection = malloc(sizeof *connection), NULL, fail);
	connection->fts = fts;
	connection->timeout = timeout;
	try(get_hostport(host, port, &connection->host, &connection->port), 1, fail2);
	try(validate_host(connection->host), 1, fail3);
	connection->state_info = (struct fts_client_connection_state_info){
		.state = FTS_CLIENT_STATE_IDLE,
		.last_operation = FTS_CLIENT_OP_NONE,
		.last_operation_waiting = false,
		.last_operation_file_name_arg = NULL,
	};
	try(connection->sock = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), NULL, fail3);
fail3:
	free(connection->host);
fail2:
	free(connection);
fail:
	return NULL;
}
extern void fts_client_connection_destroy(fts_client_connection_t connection) {
	cmn_socket2_destroy(connection->sock);
	free(connection->sock);
	free(connection);
}

extern void fts_client_connection_set_log_level(fts_client_connection_t connection, enum cmn_logger_log_level level) {
	connection->log_level = level;
}

extern fts_error_t fts_client_connection_connect(fts_client_connection_t connection) {
	char* url[sizeof("255.255.255.255:65535")];
	sprintf((char *)&url, "%s:%d", connection->host, connection->port);
	try(cmn_socket2_connect(connection->sock, (char const *)url), -1, fail);
fail:
	return FTS_ERROR_NETWORK;
}

extern void fts_client_connection_close(fts_client_connection_t connection) {
	connection->state_info = (struct fts_client_connection_state_info) {
		.state = FTS_CLIENT_STATE_IDLE,
		.last_operation = FTS_CLIENT_OP_NONE,
		.last_operation_waiting = false,
		.last_operation_file_name_arg = NULL
	};
	cmn_socket2_destroy(connection->sock);
	connection->sock = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp);
}

static int get_hostport(char const *host, int port, char **const connection_host, int *const connection_port) {
	char *_host;
	int _port;
	size_t host_length;
	host_length = strlen(host);
	try(_host = (malloc(host_length + 1)), NULL, fail);
	strcpy(_host, host);
	if (port < 0) {
		char const *i;
		char const *j;
		i = strrchr(_host, ':');
		j = strrchr(_host, ']'); // ipv6 addresses have [...]
		if (i > j) {
			if (cmn_strto_int16((int16_t *)&_port, i + 1, 10)) {
				if (*(i + 1) == '\0') { // http://foo.com:/ == http://foo.com/
					port = FTS_CLIENT_DEFAULT_PORT;
				}
				else {
					// return { .code = InvalidURL, .message = ("nonnumeric port: '%s'", i + 1) }
					goto fail2;
				}
			}
			_host[i - _host] = '\0';
		}
		else {
			port = FTS_CLIENT_DEFAULT_PORT;
		}
		if (host && host[0] == '[' && host[host_length - 1] == ']') {
			_host = _host + 1;
			_host[host_length - 1] = '\0';
		}
	}
	*connection_host = _host;
	*connection_port = port;
	return 0;
fail2:
	free(_host);
fail:
	return 1;
}

/**
 * @brief Validate a host so it doesn't contain control characters.
 */
static int validate_host(char const *host) {
	regex_t regex;
	if (regcomp(&regex, "[\\x00-\\x20\\x7f]", REG_NOSUB)) {
		// Could not compile regex
		return 1;
	}
	if (regexec(&regex, host, 0, NULL, 0) != REG_NOMATCH) {
		// "URL can't contain control characters."
		return 1;
	}
	return 0;
}
