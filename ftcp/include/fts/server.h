#ifndef FTS_SERVER_H_INCLUDED
#define FTS_SERVER_H_INCLUDED

#pragma once

#include <fts/error.h>

typedef struct fts_server *fts_server_t;

/**
 * @brief Handle a client request.
 */
extern fts_error_t fts_server_set_working_directory(fts_server_t server, char const *directory);

/**
 * @brief Handle a client request.
 * Handle one request at a time until shutdown.
 * Polls for shutdown every poll_interval seconds. Ignores
 * self.timeout. If you need to do periodic tasks, do them in
 * another thread.
 */
extern fts_error_t fts_server_forever(fts_server_t server, char const *directory);

/**
 * @brief Stops the serve_forever loop.
 * Blocks until the loop has finished. This must be called while
 * serve_forever() is running in another thread, or it will
 * deadlock.
 */
extern void fts_server_shutdown(fts_server_t server);

/**
 * @brief Handle one request, possibly blocking.
 */
extern fts_error_t fts_handle_request(fts_server_t server, char const *directory);

#endif // FTS_SERVER_H_INCLUDED
