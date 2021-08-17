#pragma once

#include "nproto_service.h"

struct cmn_nproto_service_unix {
    struct cmn_nproto_service super;
};

extern struct cmn_nproto_service* cmn_nproto_service_unix;
