#ifndef FTS_CLIENT_H_INCLUDED
#define FTS_CLIENT_H_INCLUDED

#pragma once

#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

#include <buracchi/common/logger/logger.h>
#include <fts/error.h>

/******************************************************************************
 *                            File Transfer Client                            *
 *****************************************************************************/
/**
 * The File Transfer Client defines an interface which implement the client side
 * of the FTCP protocols.
 */

/**
 * @brief An ftcp_client_connection instance represents one transaction with an
 * FTCP server.
 */
typedef struct fts_client_connection *fts_client_connection_t;

#define FTCP_PORT = 1234

/**
 * @brief
 */
typedef enum fts_client_option {
	/**
	 * @brief
	 */
	DO_NOT_REPLACE = 0,
	/**
	 * @brief
	 */
	REPLACE = 1
} fts_client_option_t;

/**
 * @brief An ftcp_client_connection instance represents one transaction with an
 * ftcp server.
 * It should be instantiated passing it a host and optional port number.
 * If no port number is passed, the port is extracted from the host string
 * if it has the form host:port, else the default FTCP port (1234) is used.
 * If the optional timeout parameter is given, blocking operations
 * (like connection attempts) will timeout after that many seconds
 * (if it is not given, the global default timeout setting is used).
 * The optional blocksize parameter sets the buffer size in bytes for sending
 * a file-like message body.
 */
extern fts_client_connection_t fts_client_connection_init(char const *host, int16_t port, const struct timeval *timeout, size_t blocksize);

extern void fts_client_connection_set_log_level(fts_client_connection_t connection, enum cmn_logger_log_level level);

/**
 * @brief Connect to the server specified when the object was created.
 * By default, this is called automatically when making a request if the client
 * does not already have a connection.
 */
extern fts_error_t fts_client_connection_connect(fts_client_connection_t connection);

/**
 * @brief Close the connection to the server.
 */
extern fts_error_t fts_client_connection_close(fts_client_connection_t connection);

/**
 * @brief Require the available file list from a server.
 *
 * @param[out] file_list A pointer to the variable that will contain a malloc'd
 *			 textual representation of the list of available files.
 *			 The value is left unchanged If an error occurs.
 */
extern fts_error_t fts_get_file_list(fts_client_connection_t session, char **file_list);

/**
 * @brief Download an available file from a server.
 */
extern fts_error_t fts_download_file(fts_client_connection_t session, char const *file_name, enum fts_client_option_t option);

/**
 * @brief Upload a file and make it available into a server.
 */
extern fts_error_t fts_upload_file(fts_client_connection_t session, char const *file_name, enum fts_client_option_t option);

#endif // FTS_CLIENT_H_INCLUDED
