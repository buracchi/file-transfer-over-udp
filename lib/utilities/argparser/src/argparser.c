#include "argparser.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "struct_argparser.h"
#include "map/linked_list_map.h"
#include "try.h"

extern int format_usage(cmn_argparser_t this);

extern cmn_argparser_t cmn_argparser_init(const char* pname, const char* pdesc) {
    cmn_argparser_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    try(this->program_name = malloc(strlen(pname) + 1), NULL, fail2);
    try(this->program_description = malloc(strlen(pdesc) + 1), NULL, fail3);
    try(this->map = (cmn_map_t)cmn_linked_list_map_init(), NULL, fail4);
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
    free(this->program_name);
    free(this->program_description);
    free(this->usage);
    // TODO: free instanciated map elements
    return cmn_map_destroy(this->map);
}

extern int cmn_argparser_set_arguments(cmn_argparser_t this, struct cmn_argparser_argument* arguments, size_t number) {
    this->args = arguments;
    this->nargs = number;
    // TODO: check if parameter are ok or throw error
    format_usage(this);
    //printf("%s", this->usage);
    return 0;
}

extern cmn_map_t cmn_argparser_parse(cmn_argparser_t this, int argc, const char** argv) {
    return this->map;
}
