#ifndef FTS_FTS_H_INCLUDED
#define FTS_FTS_H_INCLUDED

#pragma once

#include <buracchi/common/networking/socket2.h>

#include <fts/client.h>
#include <fts/server.h>
#include <fts/error.h>

/******************************************************************************
 *                            File Transfer Service                           *
 *****************************************************************************/

typedef struct fts *fts_t;

/**
 * @brief Initialize a File Transfer Service object.
 * @return A pointer to the object initialized. On error, return NULL.
 */
extern fts_t fts_init(void);

/**
 * @brief Free malloc'd memory reserved for the File Transfer Service object.
 */
extern int fts_destroy(fts_t fts);

/**
 * @brief
 */
extern fts_client_connection_t fts_create_client_connection(fts_t fts, char const *url);

/**
 * @brief
 */
extern void fts_destroy_client_connection(fts_client_connection_t session);

/**
 * @brief
 */
extern fts_server_t fts_create_server(fts_t fts, char const *server_address, char const *directory);

/**
 * @brief Free malloc'd memory reserved for the File Transfer Service Server
 * object.
 */
extern void fts_destroy_server(fts_client_connection_t session);

#endif // FTS_FTS_H_INCLUDED
