#include "argparser.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "struct_argparser.h"
#include "list/linked_list.h"
#include "map/linked_list_map.h"
#include "utilities.h"
#include "try.h"

static struct cmn_argparser_argument* match_arg(cmn_argparser_t this, int argc, const char* args, struct cmn_argparser_argument** used_arg_array);

extern cmn_map_t cmn_argparser_parse(cmn_argparser_t this, int argc, const char** argv) {
    struct cmn_argparser_argument* used_argv_array[argc];
    memset(used_argv_array, 0, argc * sizeof * used_argv_array);
    for (int i = 1; i < argc; i++) {
        if (used_argv_array[i]) {
            continue;
        }
        struct cmn_argparser_argument* matching_arg;
        matching_arg = match_arg(this, argc, argv[i], used_argv_array);
        if (matching_arg) {
            used_argv_array[i] = matching_arg;
        }
    }
    cmn_list_t error_arg_list = (cmn_list_t) cmn_linked_list_init();
    for (int i = 1; i < argc ; i++) {
        if (!used_argv_array[i]) {
            cmn_list_push_back(error_arg_list, (void*)argv[i]);
        }
        if (!cmn_list_is_empty(error_arg_list)) {
            // print error_message
            exit(EXIT_FAILURE);
        }
    }
    cmn_list_destroy(error_arg_list);
    // if required argument is missing throw error
    return this->map;
}

static struct cmn_argparser_argument* match_arg(cmn_argparser_t this, int argc, const char* args, struct cmn_argparser_argument** used_arg_array) {
    bool match_positional = (args[0] != '-');
    bool match_optional = !match_positional && args[1];
    bool match_long_flag = match_optional && (args[1] == '-') && args[2];
    for (int i = 0; i < this->nargs; i++) {
        bool is_arg_used = false;
        for (int j = 1; j < argc; j++) {
            if (used_arg_array[j] == &(this->args[i])) {
                is_arg_used = true;
                break;
            }
        }
        if (is_arg_used) {
            continue;
        }
        if (match_positional && this->args[i].name
            || match_optional && this->args[i].flag && streq(args + 1, this->args[i].flag)
            || match_long_flag && this->args[i].long_flag && streq(args + 2, this->args[i].long_flag)) {
            return &(this->args[i]);
        }
    }
    return NULL;
}
