#include "argparser.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "struct_argparser.h"
#include "list/linked_list.h"
#include "utilities.h"

static int parse_arg_n(cmn_argparser_t this, int argc, const char **argv,
                       struct cmn_argparser_argument **used_argv_array, int n);

static struct cmn_argparser_argument *match_arg(cmn_argparser_t this, int argc, const char *args,
                                                struct cmn_argparser_argument **used_arg_array);

static int handle_unrecognized_elements(cmn_argparser_t this, int argc, const char **argv,
                                        struct cmn_argparser_argument **used_argv_array);

static int handle_required_missing_elements(cmn_argparser_t this, int argc, const char **argv,
                                            struct cmn_argparser_argument **used_argv_array);

static int handle_optional_missing_elements(cmn_argparser_t this, int argc, const char **argv,
                                            struct cmn_argparser_argument **used_argv_array);

extern cmn_map_t cmn_argparser_parse(cmn_argparser_t this, int argc, const char **argv) {
    struct cmn_argparser_argument *used_argv_array[argc];
    memset(used_argv_array, 0, argc * sizeof *used_argv_array);
    for (int i = 1; i < argc; i++) {
        if (used_argv_array[i]) {
            continue;
        }
        if (parse_arg_n(this, argc, argv, used_argv_array, i)) {
            break;
        }
    }
    handle_unrecognized_elements(this, argc, argv, used_argv_array);
    handle_required_missing_elements(this, argc, argv, used_argv_array);
    handle_optional_missing_elements(this, argc, argv, used_argv_array);
    return this->map;
}

static int parse_arg_n(cmn_argparser_t this, int argc, const char **argv,
                       struct cmn_argparser_argument **used_argv_array, int n) {
    // TODO: handle al cases
    struct cmn_argparser_argument *matching_arg;
    matching_arg = match_arg(this, argc, argv[n], used_argv_array);
    if (matching_arg) {
        bool is_positional = matching_arg->name;
        bool have_flag = matching_arg->flag;
        switch (matching_arg->action) {
            case CMN_ARGPARSER_ACTION_HELP:
                fprintf(stderr, "%s\n%s", this->usage, this->usage_details);
                exit(EXIT_FAILURE);
            case CMN_ARGPARSER_ACTION_STORE:
                switch (matching_arg->action_nargs) {
                    case CMN_ARGPARSER_ACTION_NARGS_SINGLE:
                        if (is_positional) {
                            cmn_map_insert(this->map, (void *) matching_arg->name, (void *) argv[n], NULL);
                            used_argv_array[n] = matching_arg;
                        }
                        else {
                            if (n == argc - 1) {
                                return 1;
                            }
                            if (have_flag) {
                                cmn_map_insert(this->map, (void *) matching_arg->flag, (void *) argv[n + 1], NULL);
                            }
                            else {
                                cmn_map_insert(this->map, (void *) matching_arg->long_flag, (void *) argv[n + 1], NULL);
                            }
                            used_argv_array[n] = matching_arg;
                            used_argv_array[n + 1] = matching_arg;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

static struct cmn_argparser_argument *match_arg(cmn_argparser_t this, int argc, const char *args,
                                                struct cmn_argparser_argument **used_arg_array) {
    bool match_positional = (args[0] != '-');
    bool match_optional = !match_positional && args[1];
    bool match_long_flag = match_optional && (args[1] == '-') && args[2];
    for (struct cmn_argparser_argument **arg = this->args; *arg; arg++) {
        bool is_arg_used = false;
        for (int j = 1; j < argc; j++) {
            if (used_arg_array[j] == *arg) {
                is_arg_used = true;
                break;
            }
        }
        if (is_arg_used) {
            continue;
        }
        if (match_positional && (*arg)->name
            || match_optional && (*arg)->flag && streq(args + 1, (*arg)->flag)
            || match_long_flag && (*arg)->long_flag && streq(args + 2, (*arg)->long_flag)) {
            return *arg;
        }
    }
    return NULL;
}

static int handle_unrecognized_elements(cmn_argparser_t this, int argc, const char **argv,
                                        struct cmn_argparser_argument **used_argv_array) {
    cmn_list_t error_arg_list = (cmn_list_t) cmn_linked_list_init();
    for (int i = 1; i < argc; i++) {
        if (!used_argv_array[i]) {
            cmn_list_push_back(error_arg_list, (void *) argv[i]);
        }
    }
    if (!cmn_list_is_empty(error_arg_list)) {
        fprintf(stderr, "%s\n", this->usage);
        fprintf(stderr, "%s: %s", this->program_name, "error: unrecognized arguments: ");
        for (cmn_iterator_t i = cmn_list_begin(error_arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
            fprintf(stderr, "%s ", (const char *) cmn_iterator_data(i));
        }
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
    cmn_list_destroy(error_arg_list);
}

static int handle_required_missing_elements(cmn_argparser_t this, int argc, const char **argv,
                                            struct cmn_argparser_argument **used_argv_array) {
    cmn_iterator_t i;
    cmn_list_t missing_required_arg_list = (cmn_list_t) cmn_linked_list_init();
    cmn_list_t required_arg_list = (cmn_list_t) cmn_linked_list_init();
    for (struct cmn_argparser_argument **arg = this->args; *arg; arg++) {
        // TODO: the check is incomplete
        if ((*arg)->name || (*arg)->is_required) {
            cmn_list_push_back(required_arg_list, (void *) *arg);
        }
    }
    for (i = cmn_list_begin(required_arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
        struct cmn_argparser_argument *element = cmn_iterator_data(i);
        bool is_present = false;
        for (int i = 1; i < argc; i++) {
            if (used_argv_array[i] == element) {
                is_present = true;
                break;
            }
        }
        if (!is_present) {
            cmn_list_push_back(missing_required_arg_list, (void *) element);
        }
    }
    cmn_iterator_destroy(i);
    cmn_list_destroy(required_arg_list);
    if (!cmn_list_is_empty(missing_required_arg_list)) {
        fprintf(stderr, "%s\n", this->usage);
        fprintf(stderr, "%s: %s", this->program_name, "error: the following arguments are required: ");
        for (i = cmn_list_begin(missing_required_arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
            struct cmn_argparser_argument *element = cmn_iterator_data(i);
            if (element->name) {
                fprintf(stderr, "%s ", element->name);
            }
            else {
                if (element->flag) {
                    fprintf(stderr, "-%s", element->flag);
                }
                if (element->flag && element->long_flag) {
                    fprintf(stderr, "/");
                }
                if (element->long_flag) {
                    fprintf(stderr, "--%s", element->long_flag);
                }
                fprintf(stderr, " ");
            }
        }
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
    cmn_list_destroy(missing_required_arg_list);
}

static int handle_optional_missing_elements(cmn_argparser_t this, int argc, const char **argv,
                                            struct cmn_argparser_argument **used_argv_array) {
    cmn_iterator_t i;
    cmn_list_t missing_optional_arg_list = (cmn_list_t) cmn_linked_list_init();
    cmn_list_t optional_arg_list = (cmn_list_t) cmn_linked_list_init();
    for (struct cmn_argparser_argument **arg = this->args; *arg; arg++) {
        if (!(*arg)->name && !(*arg)->is_required) {
            cmn_list_push_back(optional_arg_list, (void *) *arg);
        }
    }
    for (i = cmn_list_begin(optional_arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
        struct cmn_argparser_argument *element = cmn_iterator_data(i);
        bool is_present = false;
        for (int i = 1; i < argc; i++) {
            if (used_argv_array[i] == element) {
                is_present = true;
                break;
            }
        }
        if (!is_present) {
            cmn_list_push_back(missing_optional_arg_list, (void *) element);
        }
    }
    cmn_iterator_destroy(i);
    cmn_list_destroy(optional_arg_list);
    if (!cmn_list_is_empty(missing_optional_arg_list)) {
        for (i = cmn_list_begin(missing_optional_arg_list); !cmn_iterator_end(i); cmn_iterator_next(i)) {
            struct cmn_argparser_argument *element = cmn_iterator_data(i);
            bool is_positional = element->name;
            bool have_flag = element->flag;
            cmn_map_insert(
                    this->map,
                    is_positional ?
                    (void *) element->name :
                    have_flag ?
                    (void *) element->flag :
                    (void *) element->long_flag,
                    (void *) element->default_value,
                    NULL
            );
        }
    }
    cmn_iterator_destroy(i);
    cmn_list_destroy(missing_optional_arg_list);
}
