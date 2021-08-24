#include "argparser.h"

#include <stdio.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "list/linked_list.h"
#include "struct_argparser.h"
#include "utilities.h"
#include "try.h"

static char* get_optionals_usage(cmn_list_t optionals);
static char* get_positionals_usage(cmn_list_t positionals);
static char* get_optionals_descritpion(cmn_list_t optionals);
static char* get_positionals_description(cmn_list_t positionals);
static char* get_positional_args_string(struct cmn_argparser_argument* arg);
static char* get_flag_vararg(struct cmn_argparser_argument* arg);
static char* get_narg_optional(char* vararg);
static char* get_narg_list_n(char* vararg, size_t n);
static char* get_narg_list(char* vararg);
static char* get_narg_list_optional(char* vararg);

extern int format_usage(cmn_argparser_t this) {
    char* prefix = "usage: ";
    cmn_list_t optionals = (cmn_list_t)cmn_linked_list_init();
    cmn_list_t positionals = (cmn_list_t)cmn_linked_list_init();
    for (size_t i = 0; i < this->nargs; i++) {
        if (this->args[i].name) {
            cmn_list_push_back(positionals, (void*)(&(this->args[i])));
        } else {
            cmn_list_push_back(optionals, (void*)(&(this->args[i])));
        }
    }
    char* optionals_usage = get_optionals_usage(optionals);
    char* positionals_usage = get_positionals_usage(positionals);
    char* optionals_description = get_optionals_descritpion(optionals);
    char* positionals_description = get_positionals_description(positionals);
    asprintf(
        &this->usage, 
        "%s%s%s%s\n\n%s\n%s%s", 
        prefix, 
        this->program_name, 
        optionals_usage, 
        positionals_usage ? positionals_usage : "",
        this->program_description,
        positionals_description ? positionals_description : "",
        optionals_description
    );
    cmn_list_destroy(optionals);
    cmn_list_destroy(positionals);
    return 0;
}

static char* get_optionals_usage(cmn_list_t optionals) {
    char* result;
    asprintf(&result, " %s", "[-h]");
    cmn_iterator_t iterator = cmn_list_begin(optionals);
    while (!cmn_iterator_end(iterator)) {
        struct cmn_argparser_argument* item = cmn_iterator_data(iterator);
        char* str_vararg = get_positional_args_string(item);
        asprintf(
            &result, 
            "%s %s%s%s%s%s%s", 
            result,
            !item->is_required ? "[" : "",
            item->flag ? "-" : "--",
            item->flag ? item->flag : item->long_flag,
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "",
            !item->is_required ? "]" : ""
        );
        cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    return result;
}

static char* get_positionals_usage(cmn_list_t positionals) {
    char* result = NULL;
    cmn_iterator_t iterator = cmn_list_begin(positionals);
    while (!cmn_iterator_end(iterator)) {
        struct cmn_argparser_argument* item = cmn_iterator_data(iterator);
        char* str_vararg = get_positional_args_string(item);
        asprintf(&result, "%s %s", result ? result : "", str_vararg);
        cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    return result;
}

static char* get_optionals_descritpion(cmn_list_t optionals) {
    char* result;
    asprintf(
        &result,
        "\n%s\n  %s\n",
        "optional arguments:",
        "-h, --help\t\tshow this help message and exit"
    );
    cmn_iterator_t iterator = cmn_list_begin(optionals);
    while (!cmn_iterator_end(iterator)) {
        struct cmn_argparser_argument* item = cmn_iterator_data(iterator);
        char* str_vararg = get_positional_args_string(item);
        asprintf(
            &result, 
            "%s  %s%s%s%s%s%s%s%s%s\t%s\n", 
            result,
            item->flag ? "-" : "",
            item->flag ? item->flag : "",
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "",
            item->flag ? ", " : "",
            item->long_flag ? "--" : "",
            item->long_flag ? item->long_flag : "",
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "\t",
            item->help ? item->help : ""
        );
        cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    return result;
}

static char* get_positionals_description(cmn_list_t positionals) {
    char* result = NULL;
    cmn_iterator_t iterator = cmn_list_begin(positionals);
    while (!cmn_iterator_end(iterator)) {
        struct cmn_argparser_argument* item = cmn_iterator_data(iterator);
        char* str_vararg = get_positional_args_string(item);
        asprintf(
            &result, 
            "%s  %s\t\t\t%s\n", 
            result? result : "\npositional arguments:\n",
            item->name,
            item->help ? item->help : ""
        );
        cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    return result;
}

static char* get_positional_args_string(struct cmn_argparser_argument* arg) {
    char* result;
    char* vararg;
    bool is_positional = arg->name;
    if (is_positional) {
        vararg = (char*)arg->name;
    } else {
        bool needs_arg = (arg->action == CMN_ARGPARSER_ACTION_STORE)
                        || (arg->action == CMN_ARGPARSER_ACTION_APPEND)
                        || (arg->action == CMN_ARGPARSER_ACTION_EXTEND);
        if (!needs_arg) {
            return NULL;
        }
        vararg = get_flag_vararg(arg);
    }
    switch (arg->action_nargs) {
    case CMN_ARGPARSER_ACTION_NARGS_SINGLE:
        result = vararg;
        break;
    case CMN_ARGPARSER_ACTION_NARGS_OPTIONAL:
        result = get_narg_optional(vararg);
        break;
    case CMN_ARGPARSER_ACTION_NARGS_LIST_OF_N:
        result = get_narg_list_n(vararg, arg->nargs_list_size);
        break;
    case CMN_ARGPARSER_ACTION_NARGS_LIST:
        result = get_narg_list(vararg);
        break;
    case CMN_ARGPARSER_ACTION_NARGS_LIST_OPTIONAL:
        result = get_narg_list_optional(vararg);
        break;
    default:
        return NULL;
    }
}

static char* get_flag_vararg(struct cmn_argparser_argument* arg) {
    char* result;
    if (arg->long_flag) {
        result = malloc(strlen(arg->long_flag) + 1);
        strcpy(result, arg->long_flag);
    } else {
        result = malloc(strlen(arg->flag) + 1);
        strcpy(result, arg->flag);
    }
    char *ptr = result;
    while(*ptr) {
        *ptr = (*ptr == '-') ? '_' : toupper(*ptr);
        ptr++;
    }
    return result;
}

static char* get_narg_optional(char* vararg) {
    char* result;
    size_t size = strlen(vararg) + 3;
    result = malloc(size);
    strcpy(result + 1, vararg);
    result[0] = '[';
    result[size - 2] = ']';
    result[size - 1] = 0;
    return result;
}

static char* get_narg_list_n(char* vararg, size_t n) {
    char* result;
    size_t size = strlen(vararg) * n + (n - 1) + 1;
    result = malloc(size);
    for(int i = 0; i < n; i++) {
        strcpy(result + ((strlen(vararg) + 1) * i), vararg);
        result[strlen(vararg) * (i + 1) + i] = ' ';
    }
    result[size - 1] = 0;
    return result;
}

static char* get_narg_list(char* vararg) {
    char* result;
    size_t size = strlen(vararg) * 2 + 6 + 1;
    result = malloc(size);
    strcpy(result, vararg);
    result[strlen(vararg)] = ' ';
    result[strlen(vararg) + 1] = '[';
    strcpy(result + strlen(vararg) + 2, vararg);
    result[size - 5] = '.';
    result[size - 4] = '.';
    result[size - 3] = '.';
    result[size - 2] = ']';
    result[size - 1] = 0;
    return result;
}

static char* get_narg_list_optional(char* vararg) {
    char* result = get_narg_list(vararg);
    result = get_narg_optional(result);
    return result;
}
