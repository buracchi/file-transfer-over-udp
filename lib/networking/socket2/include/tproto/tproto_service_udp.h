#pragma once

#include "tproto_service.h"

struct cmn_tproto_service_udp {
    struct cmn_tproto_service super;
};

extern struct cmn_tproto_service* cmn_tproto_service_udp;
