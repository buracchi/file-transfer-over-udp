#pragma once

#include "request_handler.h"
#include "socket2.h"
#include "tpool.h"

struct cmn_communication_manager {
    struct cmn_request_handler* handler;
    struct cmn_socket2 socket;
    struct cmn_tpool thread_pool;
};

extern int cmn_communication_manager_init(struct cmn_communication_manager* communication_manager, size_t thread_number);

extern int cmn_communication_manager_start(struct cmn_communication_manager* communication_manager, struct cmn_nproto_service* nproto_serivce, struct cmn_tproto_service* tproto_serivce, const char* url, struct cmn_request_handler* request_handler);

extern int cmn_communication_manager_stop(struct cmn_communication_manager* communication_manager);

extern int cmn_communication_manager_destroy(struct cmn_communication_manager* communication_manager);
