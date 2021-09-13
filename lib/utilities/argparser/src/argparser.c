#include "argparser.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "struct_argparser.h"
#include "try.h"

extern int format_usage(cmn_argparser_t this);

static struct cmn_argparser_argument help_arg = {
        .flag = "h",
        .long_flag = "help",
        .action = CMN_ARGPARSER_ACTION_HELP,
        .help = "show this help message and exit"
};

extern cmn_argparser_t cmn_argparser_init(const char *pname, const char *pdesc) {
    cmn_argparser_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    try(this->program_name = malloc(strlen(pname) + 1), NULL, fail2);
    try(this->program_description = malloc(strlen(pdesc) + 1), NULL, fail3);
    try(this->map = (cmn_map_t) cmn_linked_list_map_init(), NULL, fail4);
    strcpy(this->program_name, pname);
    strcpy(this->program_description, pdesc);
    return this;
fail4:
    free(this->program_description);
fail3:
    free(this->program_name);
fail2:
    free(this);
fail:
    return NULL;
}

extern int cmn_argparser_destroy(cmn_argparser_t this) {
    try(cmn_map_destroy(this->map), 1, fail);
    free(this->program_name);
    free(this->program_description);
    free(this->usage);
    free(this);
    return 0;
fail:
    return 1;
}

extern int cmn_argparser_set_arguments(cmn_argparser_t this, struct cmn_argparser_argument *arguments, size_t number) {
    size_t size = number + 2;
    try(this->args = malloc(sizeof *this->args * size), NULL, fail);
    this->args[0] = &help_arg;
    for (size_t i = 1; i < number + 1; i++) {
        this->args[i] = arguments++;
    }
    this->args[size - 1] = NULL;
    // TODO: Check and throw error if: There are conflicts, ...
    format_usage(this);
    return 0;
fail:
    return 1;
}
