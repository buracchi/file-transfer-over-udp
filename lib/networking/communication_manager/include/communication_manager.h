#pragma once

#include "request_handler.h"
#include "nproto_service.h"
#include "tproto_service.h"

typedef struct cmn_communication_manager* cmn_communication_manager_t;

extern cmn_communication_manager_t cmn_communication_manager_init(size_t thread_number);

extern int cmn_communication_manager_start(cmn_communication_manager_t communication_manager, cmn_nproto_service_t nproto_serivce, cmn_tproto_service_t tproto_serivce, const char* url, cmn_request_handler_t request_handler);

extern int cmn_communication_manager_stop(cmn_communication_manager_t communication_manager);

extern int cmn_communication_manager_destroy(cmn_communication_manager_t communication_manager);
