#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <buracchi/common/networking/socket2.h>
#include <buracchi/common/utilities/try.h>

#include <buracchi/common/networking/nproto/nproto_service_ipv4.h>
#include <buracchi/common/networking/tproto/tproto_service_tcp.h>

//#include "tproto/tproto_service_gbn.h"

#include "session.h"

extern fts_session_t fts_open_session(fts_t fts, char const *url) {
	fts_session_t session;
	try(session = malloc(sizeof *session), NULL, fail);
	try(session->url = malloc(strlen(url)), NULL, fail2);
	try(session->sock = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), NULL, fail3);
	try(cmn_socket2_connect(session->sock, url), 1, fail4);
	strcpy(session->url, url);
	session->fts = fts;
	session->state.last_operation = NONE;
	session->state.last_operation_waiting = false;
	session->state.last_operation_file_name_arg = NULL;
	session->state.last_operation_file_len_arg = 0;
fail4:
	cmn_socket2_destroy(session->sock);
fail3:
	free(session->url);
fail2:
	free(session);
fail:
	return NULL;
}

extern void fts_close_session(fts_session_t session) {
	cmn_socket2_destroy(session->sock);
	free(session->url);
	free(session);
}

extern fts_session_t fts_reset_session(fts_session_t session) {
	cmn_socket2_t socket;
	try(socket = cmn_socket2_init(cmn_nproto_service_ipv4, cmn_tproto_service_tcp), NULL, fail);
	try(cmn_socket2_connect(socket, session->url), 1, fail2);
	cmn_socket2_destroy(session->sock);
	session->sock = socket;
	session->state.last_operation = NONE;
	session->state.last_operation_waiting = false;
	session->state.last_operation_file_name_arg = NULL;
	session->state.last_operation_file_len_arg = 0;
	return session;
fail2:
	cmn_socket2_destroy(socket);
fail:
	return NULL;
}
