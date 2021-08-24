#pragma once

#include "argparser.h"
#include "map/linked_list_map.h"

struct cmn_argparser {
    char* program_name;
    char* program_description;
    char* usage;
    struct cmn_argparser_argument* args;
    size_t nargs;
    cmn_map_t map;
};
