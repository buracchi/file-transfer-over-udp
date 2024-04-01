#ifndef FTS_ERROR_H_INCLUDED
#define FTS_ERROR_H_INCLUDED

#pragma once

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
} fts_error_t;

/**
 * @brief Returns a string representing the error code's name.
 */
char const *fts_error_to_str(fts_error_t err);

#endif // FTS_ERROR_H_INCLUDED
