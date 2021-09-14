/** @file
 * 
 * @brief Argparser interface.
 * @details This file represent the public interface of the Argparser module.
 */
#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "map.h"

typedef struct cmn_argparser *cmn_argparser_t;

/**
 * @enum cmn_argparser_action
 * 
 * @brief Methods of handling command line arguments.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_STORE
 *      @brief stores the argument’s value
 *      @details (e.g. '--foo 1' will be stored as a KV pair {"foo","1"})
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_STORE_CONST
 *      @brief stores the value specified by the const_value field
 *      @details (e.g. '--foo'   will be stored as a KV pair {"foo","cmn_argparser_argument::const_value"})
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_STORE_TRUE
 *      @brief special cases of @ref CMN_ARGPARSER_ACTION_STORE_CONST
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_STORE_FALSE
 *      @brief special cases of @ref CMN_ARGPARSER_ACTION_STORE_CONST
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_APPEND
 *      @brief stores a list, and appends each argument value to the list
 *      @details (e.g. '--foo 1 --foo 2' will be stored as a KV pair {"foo",{"1", "2", NULL}})
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_APPEND_CONST
 *      @brief stores a list, and appends the value specified by the const_value argument to the list.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_COUNT
 *      @brief counts the number of times a keyword argument occurs
 *      @details (e.g. '-vvv' will be stored as a KV pair {"v",3})
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_EXTEND
 *      @brief stores a list, and extends each argument value to the list
 *      @details (e.g. '--foo 1 --foo 2 3 4' will be stored as a KV pair {"foo",{"1", "2", "3", "4", NULL}})
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_HELP
 *      @brief This prints a complete help message for all the options in the
 *       current parser and then exits.
 *      @details By default a help action is automatically added to the parser.
 */
enum cmn_argparser_action {
    CMN_ARGPARSER_ACTION_STORE,
    CMN_ARGPARSER_ACTION_STORE_CONST,
    CMN_ARGPARSER_ACTION_STORE_TRUE,
    CMN_ARGPARSER_ACTION_STORE_FALSE,
    CMN_ARGPARSER_ACTION_APPEND,
    CMN_ARGPARSER_ACTION_APPEND_CONST,
    CMN_ARGPARSER_ACTION_COUNT,
    CMN_ARGPARSER_ACTION_EXTEND,
    CMN_ARGPARSER_ACTION_HELP
};

/**
 * @enum cmn_argparser_action_nargs
 * 
 * @brief Methods of associating command line arguments with a single action.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_NARGS_SINGLE
 *      @brief One argument will be consumed from the command line and produced
 *       as a single item.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_NARGS_OPTIONAL
 *      @brief One argument will be consumed from the command line if possible,
 *       and produced as a single item.
 *      @details If no command-line argument is present, the value from @ref
 *       default_value will be produced. Note that for optional arguments, there
 *       is an additional case - the option string is present but not followed
 *       by a command-line argument. In this case the value from @ref
 *       const_value will be produced.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_NARGS_LIST_OF_N
 *      @brief N arguments from the command line will be gathered together into
 *       a NULL terminated list.
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_NARGS_LIST
 *      @brief special cases of @ref CMN_ARGPARSER_ACTION_STORE_CONST
 * 
 * @var cmn_argparser_action::CMN_ARGPARSER_ACTION_NARGS_LIST
 *      @brief special cases of @ref CMN_ARGPARSER_ACTION_STORE_CONST
 *      @details the parsing won't fail if no argument is provided.
 * 
 */
enum cmn_argparser_action_nargs {
    CMN_ARGPARSER_ACTION_NARGS_SINGLE,
    CMN_ARGPARSER_ACTION_NARGS_OPTIONAL,
    CMN_ARGPARSER_ACTION_NARGS_LIST_OF_N,
    CMN_ARGPARSER_ACTION_NARGS_LIST,
    CMN_ARGPARSER_ACTION_NARGS_LIST_OPTIONAL,
};

/**
 * @struct cmn_argparser_argument
 * 
 * @brief Argument item.
 *  
 * @var cmn_argparser_argument::name
 *      @brief Specify the name of a positional argument (like a value or a list
 *       of filenames).
 * 
 * @var cmn_argparser_argument::flag
 *      @brief Specify a character flag identifying an option which will be
 *       preceded by a hyphen.
 *      @details If name is not NULL this field will be ignored during parsing.
 * 
 * @var cmn_argparser_argument::long_flag
 *      @brief Specify a name flag identifying an option which will be preceded
 *       by two hyphens.
 *      @details If name is not NULL this field will be ignored during parsing.
 * 
 * @var cmn_argparser_argument::is_required
 *      @brief Specify if an option is required, @ref cmn_argparser_parse will
 *       report an error if that option is not present at the command line.
 *      @details If name is not NULL this field will be ignored during parsing.
 *       The default value of this field is false.
 * 
 * @var cmn_argparser_argument::action
 *      @brief Specify how the argument should be handled.
 *      @details Argparser objects associate command-line arguments with
 *       actions. These actions can do just about anything with the command-line
 *       arguments associated with them, though most actions simply add a value
 *       into the map returned by \ref cmn_argparser_parse().
 *       The default action is @ref CMN_ARGPARSER_ACTION_STORE.
 * 
 * @var cmn_argparser_argument::action_nargs
 *      @brief Specify how to associate different number of command-line
 *       arguments with a single action. 
 *      @details If @ref CMN_ARGPARSER_ACTION_NARGS_SINGLE or 
 *       @ref CMN_ARGPARSER_ACTION_NARGS_OPTIONAL are specified then the
 *       parser will produce a single item. If @ref
 *       CMN_ARGPARSER_ACTION_NARGS_LIST_OF_N or @ref
 *       CMN_ARGPARSER_ACTION_NARGS_LIST are specified the parser will gather
 *       together the arguments from the command line into a NULL terminated
 *       string array.
 * 
 * @var cmn_argparser_argument::nargs_list_size
 *      @brief Specify the number of elements that will be gathered together
 *       form the command line if @ref CMN_ARGPARSER_ACTION_NARGS_SINGLE is
 *       selected as the value of the @ref action_nargs field.
 *      @details This field is ignored if the @ref action_nargs field constains
 *       a value different from @ref CMN_ARGPARSER_ACTION_STORE.
 * 
 * @var cmn_argparser_argument::const_value
 *      @brief Specify the value used when the action is set to
 *       @ref CMN_ARGPARSER_ACTION_STORE_CONST. The default value is NULL.
 * 
 * @var cmn_argparser_argument::default_value
 *      @brief Specify what value should be used if the command-line argument
 *       is not present. an error message will be generated if there wasn’t at
 *       least one command-line argument present if the argument was positional
 *       or if it was an option with the required field set to true.
 * 
 * @var cmn_argparser_argument::choices
 *      @brief Specify a NULL terminated array of acceptable values, @ref
 *       cmn_argparser_parse will report an error if the argument was not one
 *       of the acceptable values.
 * 
 * @var cmn_argparser_argument::help
 *      @brief A string containing a brief description of the argument showed
 *       when a user requests help.
 * 
 * @var cmn_argparser_argument::destination
 *      @brief Specify the identifier that will be used to retrive the argument
 *       supplied.
 *      @details The destination attribute allows to retrive the argument 
 *       supplied. For positional argument, the default value is equal to name.
 *       For optional argument, the default value is inferred from the flags
 *       strings taking the first long option string and stripping away the
 *       initial -- string. If no long option strings were supplied, dest will
 *       be derived from the first short option string by stripping the initial
 *       - character. Any internal - characters will be converted to _
 *       characters to make sure the string is a valid attribute name.
 */
struct cmn_argparser_argument {
    const char *name;
    const char *flag;
    const char *long_flag;
    bool is_required;
    enum cmn_argparser_action action;
    enum cmn_argparser_action_nargs action_nargs;
    size_t nargs_list_size;
    void *const_value;
    void *default_value;
    char **choices;
    const char *help;
    const char *destination;
};

/**
 * @brief 
 * 
 * @param pname 
 * @param pdesc 
 * @return cmn_argparser_t 
 */
extern cmn_argparser_t cmn_argparser_init(const char *pname, const char *pdesc);

/**
 * @brief 
 * 
 * @param argparser 
 * @return int 
 */
extern int cmn_argparser_destroy(cmn_argparser_t argparser);

/**
 * @brief 
 * 
 * @param argparser 
 * @param arguments 
 * @param number 
 * @return int 
 */
extern int cmn_argparser_set_arguments(cmn_argparser_t argparser, struct cmn_argparser_argument *arguments,
                                       size_t number);

/**
 * @brief 
 * 
 * @param argparser 
 * @param argc 
 * @param argv 
 * @return cmn_map_t 
 */
extern cmn_map_t cmn_argparser_parse(cmn_argparser_t argparser, int argc, const char **argv);
