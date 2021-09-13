#pragma once

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_socket2 *cmn_socket2_t;

typedef struct cmn_nproto_service *cmn_nproto_service_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern int cmn_nproto_service_set_address(cmn_nproto_service_t service, cmn_socket2_t socket, const char *url);
