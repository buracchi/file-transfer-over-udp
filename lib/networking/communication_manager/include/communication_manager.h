#pragma once

#include "request_handler.h"
#include "nproto_service.h"
#include "tproto_service.h"

typedef struct cmn_communication_manager *cmn_communication_manager_t;

extern cmn_communication_manager_t cmn_communication_manager_init(size_t thread_number);

extern void cmn_communication_manager_set_nproto(cmn_communication_manager_t communication_manager,
                                                 cmn_nproto_service_t nproto_service);

extern void cmn_communication_manager_set_tproto(cmn_communication_manager_t communication_manager,
                                                 cmn_tproto_service_t tproto_serivce);

extern int cmn_communication_manager_start(cmn_communication_manager_t communication_manager, const char *url,
                                           cmn_request_handler_t request_handler);

extern int cmn_communication_manager_stop(cmn_communication_manager_t communication_manager);

extern int cmn_communication_manager_destroy(cmn_communication_manager_t communication_manager);
