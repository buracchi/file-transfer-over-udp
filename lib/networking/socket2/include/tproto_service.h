#pragma once

#include <stddef.h>
#include <stdint.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_socket2* cmn_socket2_t;

typedef cmn_tproto_service_t cmn_tproto_service_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern ssize_t cmn_tproto_service_peek(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t* buff, uint64_t n);

extern ssize_t cmn_tproto_service_recv(cmn_tproto_service_t service, cmn_socket2_t socket, uint8_t* buff, uint64_t n);

extern ssize_t cmn_tproto_service_send(cmn_tproto_service_t service, cmn_socket2_t socket, const uint8_t* buff, uint64_t n);
