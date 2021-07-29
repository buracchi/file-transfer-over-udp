#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "struct_socket2.h"
#include "sockaddr2.h"
#include "tproto.h"
#include "nproto.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct socket2;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Return an initialized unbound socket object.
*
* @return	the initialized socket on success; NULL otherwise.
*/
extern int socket2_init(struct socket2* this, struct tproto* tproto, struct nproto* nproto, bool get_fd);

/*
* Destroys the socket object.
*
* This function never fails.
*
* @param	handle	-	the socket object.
*
* @return	This function returns no value.
*/
static int socket2_destroy(struct socket2* socket2) {
	return socket2->__ops_vptr->destroy(socket2);
}

/*
* Extracts the first connection on the queue of pending connections, creates a
* new socket with the same specified socket protocols and allocates a new file 
* descriptor for that socket.
* 
* @param	handle	-	the socket object.
*
* @return	the accepted socket on success; NULL otherwise.
*/
static struct socket2* socket2_accept(struct socket2* socket2) {
	return socket2->__ops_vptr->accept(socket2);
}

/*
* Requests a connection to be made on the socket object.
* 
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise and errno is set to indicate the error.
*/
static int socket2_connect(struct socket2* socket2, const char* url) {
	return socket2->__ops_vptr->connect(socket2, url);
}

/*
* Bind a name to a socket, listen for socket connections and limit the queue of
* incoming connections.
*
* @return	1 and set properly errno on error
*/
static int socket2_listen(struct socket2* socket2, const char* url, int backlog) {
	return socket2->__ops_vptr->listen(socket2, url, backlog);
}

/*
* Peek a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
*
* @return	the number of byte read or -1 on error
*/
static ssize_t socket2_peek(struct socket2* socket2, uint8_t* buff, uint64_t n) {
	return socket2->__ops_vptr->peek(socket2, buff, n);
}

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
* 
* @return	the number of byte read or -1 on error
*/
static ssize_t socket2_recv(struct socket2* socket2, uint8_t* buff, uint64_t n) {
	return socket2->__ops_vptr->recv(socket2, buff, n);
}

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the malloc'd string
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t socket2_srecv(struct socket2* socket2, char** buff);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t socket2_frecv(struct socket2* socket2, FILE* file, long fsize);

/*
* Send n bytes form a connected socket.
* 
* @return number of bytes sent.
*/
static ssize_t socket2_send(struct socket2* socket2, const uint8_t* buff, uint64_t n) {
	return socket2->__ops_vptr->send(socket2, buff, n);
}

/*
* Send a string form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t socket2_ssend(struct socket2* socket2, const char* string);

/*
* Send a file form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t socket2_fsend(struct socket2* socket2, FILE* file);

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
extern int socket2_get_fd(struct socket2* socket2);

/*
* Set whether the socket is blocking or non blocking.
*
* @param	handle	-	the socket object.
* @param	blocking
*
* @return	0 on success; 1 otherwise.
*/
extern int socket2_set_blocking(struct socket2* socket2, bool blocking);
