#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "nproto_service.h"
#include "tproto_service.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_socket2 *cmn_socket2_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Return an initialized unbound socket object.
*
* @return	the initialized socket on success; NULL otherwise.
*/
extern cmn_socket2_t cmn_socket2_init(cmn_nproto_service_t nproto_serivce, cmn_tproto_service_t tproto_serivce);

/*
* Destroys the socket object.
*
* This function never fails.
*
* @param	handle	-	the socket object.
*
* @return	This function returns no value.
*/
extern int cmn_socket2_destroy(cmn_socket2_t socket);

/*
* Extracts the first connection on the queue of pending connections, creates a
* new socket with the same specified socket protocols and allocates a new file 
* descriptor for that socket.
* 
* @param	handle	-	the socket object.
*
* @return	the accepted socket on success; NULL otherwise.
*/
extern cmn_socket2_t cmn_socket2_accept(cmn_socket2_t socket);

/*
* Requests a connection to be made on the socket object.
* 
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise and errno is set to indicate the error.
*/
extern int cmn_socket2_connect(cmn_socket2_t socket, const char *url);

/*
* Bind a name to a socket, listen for socket connections and limit the queue of
* incoming connections.
*
* @return	1 and set properly errno on error
*/
extern int cmn_socket2_listen(cmn_socket2_t socket, const char *url, int backlog);

/*
* Peek a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_peek(cmn_socket2_t socket, uint8_t *buff, uint64_t n);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
* 
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_recv(cmn_socket2_t socket, uint8_t *buff, uint64_t n);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the malloc'd string
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_srecv(cmn_socket2_t socket, char **buff);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_frecv(cmn_socket2_t socket, FILE *file, long fsize);

/*
* Send n bytes form a connected socket.
* 
* @return number of bytes sent.
*/
extern ssize_t cmn_socket2_send(cmn_socket2_t socket, const uint8_t *buff, uint64_t n);

/*
* Send a string form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t cmn_socket2_ssend(cmn_socket2_t socket, const char *string);

/*
* Send a file form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t cmn_socket2_fsend(cmn_socket2_t socket, FILE *file);

/*******************************************************************************
*                             Modifiers functions                              *
*******************************************************************************/

/*
* Get the file descriptor of the socket object.
*
* @param	handle	-	the socket object.
*
* @return	the socket file descriptor on success.
*/
extern int cmn_socket2_get_fd(cmn_socket2_t socket);

/*
* Set whether the socket is blocking or non-blocking.
*
* @param	handle	-	the socket object.
* @param	blocking
*
* @return	0 on success; 1 otherwise.
*/
extern int cmn_socket2_set_blocking(cmn_socket2_t socket, bool blocking);
