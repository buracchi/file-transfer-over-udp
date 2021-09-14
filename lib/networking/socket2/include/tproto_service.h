#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_socket2 *cmn_socket2_t;

typedef struct cmn_tproto_service *cmn_tproto_service_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern int cmn_tproto_service_accept(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr,
                                     socklen_t *addr_len);

extern int cmn_tproto_service_connect(cmn_tproto_service_t service, cmn_socket2_t socket, struct sockaddr *addr,
                                      socklen_t addr_len);

extern int cmn_tproto_service_listen(cmn_tproto_service_t service, cmn_socket2_t socket, int backlog);

extern ssize_t cmn_tproto_service_peek(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n);

extern ssize_t cmn_tproto_service_recv(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t *buff, uint64_t n);

extern ssize_t cmn_tproto_service_send(cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t *buff,
                                       uint64_t n);

extern int cmn_tproto_service_close(cmn_tproto_service_t service, cmn_socket2_t socket);
