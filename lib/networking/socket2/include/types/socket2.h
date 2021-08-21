#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "nproto_service.h"
#include "tproto_service.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_socket2 {
    int fd;
    bool is_non_block;
    bool is_cloexec;
    socklen_t addrlen;
    struct sockaddr* address;
    struct cmn_nproto_service* nproto_service;
    struct cmn_tproto_service* tproto_service;
};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Return an initialized unbound socket object.
*
* @return	the initialized socket on success; NULL otherwise.
*/
extern int cmn_socket2_init(struct cmn_socket2* socket, struct cmn_nproto_service* nproto_serivce, struct cmn_tproto_service* tproto_serivce);

/*
* Destroys the socket object.
*
* This function never fails.
*
* @param	handle	-	the socket object.
*
* @return	This function returns no value.
*/
extern int cmn_socket2_destroy(struct cmn_socket2* socket);

/*
* Extracts the first connection on the queue of pending connections, creates a
* new socket with the same specified socket protocols and allocates a new file 
* descriptor for that socket.
* 
* @param	handle	-	the socket object.
*
* @return	the accepted socket on success; NULL otherwise.
*/
extern int cmn_socket2_accept(struct cmn_socket2* socket, struct cmn_socket2* accepted);

/*
* Requests a connection to be made on the socket object.
* 
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise and errno is set to indicate the error.
*/
extern int cmn_socket2_connect(struct cmn_socket2* socket, const char* url);

/*
* Bind a name to a socket, listen for socket connections and limit the queue of
* incoming connections.
*
* @return	1 and set properly errno on error
*/
extern int cmn_socket2_listen(struct cmn_socket2* socket, const char* url, int backlog);

/*
* Peek a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
*
* @return	the number of byte read or -1 on error
*/
static inline ssize_t cmn_socket2_peek(struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return socket->tproto_service->__ops_vptr->peek(socket->tproto_service, socket, buff, n);
}

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the message
* 
* @return	the number of byte read or -1 on error
*/
static inline ssize_t cmn_socket2_recv(struct cmn_socket2* socket, uint8_t* buff, uint64_t n) {
    return socket->tproto_service->__ops_vptr->recv(socket->tproto_service, socket, buff, n);
}

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer which will contain the malloc'd string
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_srecv(struct cmn_socket2* socket, char** buff);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
*
* @return	the number of byte read or -1 on error
*/
extern ssize_t cmn_socket2_frecv(struct cmn_socket2* socket, FILE* file, long fsize);

/*
* Send n bytes form a connected socket.
* 
* @return number of bytes sent.
*/
static inline ssize_t cmn_socket2_send(struct cmn_socket2* socket, const uint8_t* buff, uint64_t n) {
    return socket->tproto_service->__ops_vptr->send(socket->tproto_service, socket, buff, n);
}

/*
* Send a string form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t cmn_socket2_ssend(struct cmn_socket2* socket, const char* string);

/*
* Send a file form a connected socket.
*
* @return number of bytes sent.
*/
extern ssize_t cmn_socket2_fsend(struct cmn_socket2* socket, FILE* file);

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
extern int cmn_socket2_get_fd(struct cmn_socket2* socket);

/*
* Set whether the socket is blocking or non blocking.
*
* @param	handle	-	the socket object.
* @param	blocking
*
* @return	0 on success; 1 otherwise.
*/
extern int cmn_socket2_set_blocking(struct cmn_socket2* socket, bool blocking);
