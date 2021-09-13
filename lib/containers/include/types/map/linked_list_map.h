#pragma once

#include "map/linked_list_map.h"

#include "list/linked_list.h"
#include "types/map.h"

struct cmn_linked_list_map {
    struct cmn_map super;
    cmn_linked_list_t list;
};

extern int cmn_linked_list_map_ctor(cmn_linked_list_map_t map);
