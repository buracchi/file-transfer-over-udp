#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef void* socket2_t;

enum transport_protocol {TCP, UDP, RAW};
enum network_protocol {IPV4, IPV6, UNIX};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Return an initialized unbound socket object.
*
* @return	the initialized socket on success; NULL otherwise.
*/
extern socket2_t socket2_init(enum transport_protocol tproto, enum network_protocol nproto);

/*
* Destroys the socket object.
*
* This function never fails.
*
* @param	handle	-	the socket object.
*
* @return	This function returns no value.
*/
extern void socket2_destroy(const socket2_t handle);

/*
* Extracts the first connection on the queue of pending connections, creates a
* new socket with the same specified socket protocols and allocates a new file 
* descriptor for that socket.
* 
* @param	handle	-	the socket object.
*
* @return	the accepted socket on success; NULL otherwise.
*/
extern socket2_t socket2_accept(const socket2_t handle);

/*
* Requests a connection to be made on the socket object.
* 
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise and errno is set to indicate the error.
*/
extern int socket2_connect(const socket2_t handle);

/*
* Close the socket
*
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 and set properly errno on error
*/
extern int socket2_close(const socket2_t handle);

/*
* Receive a message from a connected socket
*
* @param	handle	-	the socket object.
* @param	buff	-	pointer wich will contain the malloc'd message
* 
* @return	the number of byte read or -1 on error
*/
extern ssize_t socket2_recv(const socket2_t handle, char** buff);

/*
* return number of bytes sended
*/
extern ssize_t socket2_send(const socket2_t handle, const char* buff);

/*
* Bind a name to a socket, listen for socket connections and limit the queue of 
* incoming connections.
*
* @return	1 and set properly errno on error
*/
extern int socket2_listen(const socket2_t handle, int backlog);

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
extern int socket2_get_fd(const socket2_t handle);

/*
* Set whether the socket is blocking or non blocking.
*
* @param	handle	-	the socket object.
* @param	blocking
*
* @return	0 on success; 1 otherwise.
*/
extern int socket2_set_blocking(const socket2_t handle, bool blocking);

/*
* Assigns an ipv4 address to an unnamed socket.
*
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise.
*/
extern int socket2_ipv4_setaddr(const socket2_t handle, const char* address, const uint16_t port);

#ifdef __unix__
/*
* Assigns an unix address to an unnamed socket.
*
* @param	handle	-	the socket object.
*
* @return	0 on success; 1 otherwise.
*/
extern int socket2_unix_setaddr(const socket2_t handle, const char* address);
#endif
