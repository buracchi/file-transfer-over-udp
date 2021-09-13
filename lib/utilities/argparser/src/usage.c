#include "argparser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "list/linked_list.h"
#include "struct_argparser.h"
#include "utilities.h"

static int get_messages(cmn_list_t optionals, char **usage, char **description,
                        int (*get_usage)(struct cmn_argparser_argument *arg, char **str, char *str_vararg),
                        int (*get_descritpion)(struct cmn_argparser_argument *arg, char **str, char *str_vararg));

static int get_optionals_usage(struct cmn_argparser_argument *arg, char **str, char *str_vararg);

static int get_optionals_descritpion(struct cmn_argparser_argument *arg, char **str, char *str_vararg);

static int get_positionals_usage(struct cmn_argparser_argument *arg, char **str, char *str_vararg);

static int get_positionals_description(struct cmn_argparser_argument *arg, char **str, char *str_vararg);

static char *get_positional_args_string(struct cmn_argparser_argument *arg);

static char *get_flag_vararg(struct cmn_argparser_argument *arg);

static char *get_narg_optional(char *vararg);

static char *get_narg_list_n(char *vararg, size_t n);

static char *get_narg_list(char *vararg);

static char *get_narg_list_optional(char *vararg);

extern int format_usage(cmn_argparser_t this) {
    int ret;
    char *prefix = "usage: ";
    char *optionals_usage;
    char *positionals_usage;
    char *optionals_description;
    char *positionals_description;
    cmn_list_t optionals = (cmn_list_t) cmn_linked_list_init();
    cmn_list_t positionals = (cmn_list_t) cmn_linked_list_init();
    for (struct cmn_argparser_argument **arg = this->args; *arg; arg++) {
        if ((*arg)->name) {
            cmn_list_push_back(positionals, (void *) *arg);
        }
        else {
            cmn_list_push_back(optionals, (void *) *arg);
        }
    }
    get_messages(optionals, &optionals_usage, &optionals_description, get_optionals_usage, get_optionals_descritpion);
    get_messages(positionals, &positionals_usage, &positionals_description, get_positionals_usage,
                 get_positionals_description);
    ret = asprintf(&this->usage, "%s%s%s%s", prefix, this->program_name, optionals_usage ? optionals_usage : "",
                   positionals_usage ? positionals_usage : "");
    ret = asprintf(&this->usage_details, "\n%s\n%s%s", this->program_description,
                   positionals_description ? positionals_description : "",
                   optionals_description ? optionals_description : "");
    free(optionals_usage);
    free(positionals_usage);
    free(optionals_description);
    free(positionals_description);
    cmn_list_destroy(optionals);
    cmn_list_destroy(positionals);
    return ret;
}

static int get_messages(cmn_list_t arg_list, char **usage, char **description,
                        int (*get_usage)(struct cmn_argparser_argument *arg, char **str, char *str_vararg),
                        int (*get_descritpion)(struct cmn_argparser_argument *arg, char **str, char *str_vararg)) {
    cmn_iterator_t i;
    *usage = NULL;
    *description = NULL;
    for (i = cmn_list_begin(arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
        struct cmn_argparser_argument *item = cmn_iterator_data(i);
        char *str_vararg = get_positional_args_string(item);
        char *old_usage = *usage;
        char *old_description = *description;
        get_usage(item, usage, str_vararg);
        get_descritpion(item, description, str_vararg);
        free(str_vararg);
        free(old_usage);
        free(old_description);
    }
    cmn_iterator_destroy(i);
    return 0;
}

static inline int get_optionals_usage(struct cmn_argparser_argument *arg, char **str, char *str_vararg) {
    return asprintf(
            str,
            "%s %s%s%s%s%s%s",
            *str ? *str : "",
            !arg->is_required ? "[" : "",
            arg->flag ? "-" : "--",
            arg->flag ? arg->flag : arg->long_flag,
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "",
            !arg->is_required ? "]" : ""
    );
}

static inline int get_optionals_descritpion(struct cmn_argparser_argument *arg, char **str, char *str_vararg) {
    return asprintf(
            str,
            "%s  %s%s%s%s%s%s%s%s%s\t%s\n",
            *str ? *str : "\noptional arguments:\n",
            arg->flag ? "-" : "",
            arg->flag ? arg->flag : "",
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "",
            arg->flag ? ", " : "",
            arg->long_flag ? "--" : "",
            arg->long_flag ? arg->long_flag : "",
            str_vararg ? " " : "",
            str_vararg ? str_vararg : "\t",
            arg->help ? arg->help : ""
    );
}

static inline int get_positionals_usage(struct cmn_argparser_argument *arg, char **str, char *str_vararg) {
    return asprintf(str, "%s %s", *str ? *str : "", str_vararg);
}

static inline int get_positionals_description(struct cmn_argparser_argument *arg, char **str, char *str_vararg) {
    return asprintf(str, "%s  %s\t\t\t%s\n", *str ? *str : "\npositional arguments:\n", arg->name,
                    arg->help ? arg->help : "");
}

static char *get_positional_args_string(struct cmn_argparser_argument *arg) {
    char *result;
    char *vararg;
    bool is_positional = arg->name;
    if (is_positional) {
        asprintf(&vararg, "%s", arg->name);
    }
    else {
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

static char *get_flag_vararg(struct cmn_argparser_argument *arg) {
    char *result;
    if (arg->long_flag) {
        asprintf(&result, "%s", arg->long_flag);
    }
    else {
        asprintf(&result, "%s", arg->flag);
    }
    char *ptr = result;
    while (*ptr) {
        *ptr = (*ptr == '-') ? '_' : toupper(*ptr);
        ptr++;
    }
    return result;
}

static char *get_narg_optional(char *vararg) {
    char *result;
    size_t size = strlen(vararg) + 3;
    result = malloc(size);
    strcpy(result + 1, vararg);
    result[0] = '[';
    result[size - 2] = ']';
    result[size - 1] = 0;
    return result;
}

static char *get_narg_list_n(char *vararg, size_t n) {
    char *result;
    size_t size = strlen(vararg) * n + (n - 1) + 1;
    result = malloc(size);
    for (int i = 0; i < n; i++) {
        strcpy(result + ((strlen(vararg) + 1) * i), vararg);
        result[strlen(vararg) * (i + 1) + i] = ' ';
    }
    result[size - 1] = 0;
    return result;
}

static char *get_narg_list(char *vararg) {
    char *result;
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

static char *get_narg_list_optional(char *vararg) {
    char *result = get_narg_list(vararg);
    result = get_narg_optional(result);
    return result;
}
