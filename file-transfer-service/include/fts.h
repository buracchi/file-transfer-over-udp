#ifndef FTS_H_INCLUDED
#define FTS_H_INCLUDED

#pragma once

#include <buracchi/common/networking/socket2.h>

/******************************************************************************
 *                            File Transfer Service                           *
 *****************************************************************************/

 /**
 * @brief
 */
typedef struct fts *fts_t;

/**
* @brief The state for the fts functions.
*
* @remarks This is a complete object, but none of its members should be
*	    accessed or relied upon in any way, shape or form.
*	    If you do so, it is Undefined Behavior.
*/
typedef struct fts_state {
	/*
	* @brief Private, do not access.
	*/
	unsigned char state_data;
} fts_state_t;

/**
* @brief
*/
enum fts_option {
	/**
	* @brief
	*/
	DO_NOT_REPLACE = 0,
	/**
	* @brief
	*/
	REPLACE = 1
};

/**
* @brief The various errors that can be returned from the File Transfer Service
*	  functions.
*/
typedef enum fts_error {
	/**
	* @brief Returned when the function is successful and nothing has
	*	  gone wrong.
	*/
	FTS_ERROR_SUCCESS = 0,
	/**
	* @brief
	*/
	FTS_ERROR_NETWORK = -1,
	/**
	* @brief
	*/
	FTS_ERROR_FILE_ALREADY_EXISTS = -2,
	/**
	* @brief
	*/
	FTS_ERROR_FILE_NOT_EXISTS = -3,
	/**
	* @brief
	*/
	FTS_ERROR_IO_ERROR = -4,
	/**
	* @brief
	*/
	FTS_ERROR_ENOMEM = -5,
	/**
	* @brief
	*/
	FTS_ERROR_LOCK_ERROR = -6,
} fts_error;

/*
* @brief Returns a string representing the error code's name.
*/
const char *fts_error_to_str(fts_error err);

/*
* @brief Initialize a File Transfer Serive object.
*/
extern fts_t fts_init(void);

/*
* @brief Require the available file list froma a server.
*
* @param[out] file_list A pointer to the variable that will contain a malloc'd
*			 textual representation of the list of available files.
*			 The value is left unchanged If an error occurs.
*/
extern fts_error fts_get_file_list(fts_t fts, cmn_socket2_t socket, char **const file_list);

/*
* @brief Download an available file from a server.
*
* @param[in, out] state A pointer to the function state. If this is NULL,
*			 a value-initialized (`= {0}` or similar) fts_state_t
*			 is used.
*/
extern fts_error fts_download_file(fts_t fts, cmn_socket2_t socket, char const *filename, enum fts_option option, fts_state_t *state);

/*
* @brief Upload a file and make it available into a server.
*
* @param[in, out] state A pointer to the function state. If this is NULL,
*			 a value-initialized (`= {0}` or similar) fts_state_t
*			 is used.
*/
extern fts_error fts_upload_file(fts_t fts, cmn_socket2_t socket, char const *filename, enum fts_option option, fts_state_t *state);

/*
* @brief Handle a client request.
*/
extern fts_error fts_handle_request(fts_t fts, cmn_socket2_t socket, char const *directory);

/*
* @brief Free malloc'd memory reserved for the File Transfer Service object.
*/
extern int fts_destroy(fts_t fts);

#endif // FTS_H_INCLUDED
