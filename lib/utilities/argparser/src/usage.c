#include "argparser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "struct_argparser.h"
#include "utilities.h"
#include "try.h"

extern int set_usage_message(cmn_argparser_t this);
static int get_argument_list(cmn_argparser_t this, char** list);
static int get_positional_arguments_helper_message(cmn_argparser_t this, char** message);
static int get_optional_arguments_helper_message(cmn_argparser_t this, char** message);


extern int set_usage_message(cmn_argparser_t this) {
    char* argument_list;
    char* positional_arguments_helper_message;
    char* optional_arguments_helper_message;
    get_argument_list(this, &argument_list);
    get_positional_arguments_helper_message(this, &positional_arguments_helper_message);
    get_optional_arguments_helper_message(this, &optional_arguments_helper_message);
    asprintf(
        &(this->usage), 
        "usage: %s [-h] %s\n\n%s\n\n%s%s\n", 
        this->program_name,
        argument_list,
        this->program_description,
        positional_arguments_helper_message,
        optional_arguments_helper_message
    );
    return 0;
}

static int get_argument_list(cmn_argparser_t this, char** list) {
    char* result = NULL;
    char* tmp = NULL;
    for (size_t i = 0; i < this->argnum; i++) {
        free(result);
        const char* varname;
        bool is_optional = false;
        bool is_required = false;
        bool is_flag = false;
        bool is_long_flag = false;
        if (this->arguments[i].name) {
            varname = this->arguments[i].name;
            is_optional = this->arguments[i].optional_args;
        } else {
            if (this->arguments[i].flag) {
                varname = this->arguments[i].flag;
                is_flag = true;
            } else {
                varname = this->arguments[i].long_flag;
                is_long_flag = true;
            }
            is_optional = this->arguments[i].optional_args;
            is_required = this->arguments[i].is_required;
        }
        asprintf(
            &result, 
            "%s%s%s%s%s%s%s%s ",
            tmp ? tmp : "",
            is_optional ? "[" : "",
            is_required ? "<" : "",
            is_flag ? "-" : "",
            is_long_flag ? "--" : "",
            varname,
            is_optional ? "]" : "",
            is_required ? ">" : ""
        );
        free(tmp);
        asprintf(&tmp, "%s", result);
    }
    free(tmp);
    int ret = asprintf(list, "%s", result);
    free(result);
    return ret;
}

static int get_positional_arguments_helper_message(cmn_argparser_t this, char** message) {
    int counter = 0;
    char* result = NULL;
    char* tmp = NULL;
    for (size_t i = 0; i < this->argnum; i++) {
        if (this->arguments[i].name) {
            counter++;
            free(result);
            asprintf(
                &result, 
                " %s%s\t\t%s\n",
                tmp ? tmp : "",
                this->arguments[i].name,
                this->arguments[i].help ? this->arguments[i].help : ""
            );
            free(tmp);
            asprintf(&tmp, "%s", result);
        }
    }
    free(tmp);
    int ret;
    if (counter) {
        ret = asprintf(message, "positional arguments:\n%s\n", result);
    } else {
        ret = asprintf(message, "");
    }
    free(result);
    return ret;
}

static int get_optional_arguments_helper_message(cmn_argparser_t this, char** message) {
    int counter = 0;
    char* result = NULL;
    char* tmp = NULL;
    for (size_t i = 0; i < this->argnum; i++) {
        if (!this->arguments[i].name) {
            counter++;
            free(result);
            asprintf(
                &result, 
                "%s %s%s%s%s\t%s\n",
                tmp ? tmp : "",
                this->arguments[i].flag ? "-" : "",
                this->arguments[i].flag ? this->arguments[i].flag : "",
                this->arguments[i].long_flag ? (this->arguments[i].flag ? ", --" : "--") : "\t",
                this->arguments[i].long_flag ? this->arguments[i].long_flag : "",
                this->arguments[i].help ? this->arguments[i].help : ""
            );
            free(tmp);
            asprintf(&tmp, "%s", result);
        }
    }
    free(tmp);
    int ret;
    if (counter) {
        ret = asprintf(message, "optional arguments:\n -h, --help \tshow this help message and exit\n%s", result);
    } else {
        ret = asprintf(message, "optional arguments:\n -h, --help \tshow this help message and exit");
    }
    free(result);
    return ret;
}
