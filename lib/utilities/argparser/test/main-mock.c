#include "main-mock.h"

#include "argparser.h"
#include "map.h"

const char *mock_main(int argc, const char **argv) {
    const char *result;
    cmn_map_t option_map;
    cmn_argparser_t argparser;
    struct cmn_argparser_argument args[] = {
            {.name = "arg", .help="an important arguemnt", 0},
            {.flag = "f", .long_flag="foo", .is_required=true, .help="set the ammount of foo data"},
            {.long_flag="set-bar", .action=CMN_ARGPARSER_ACTION_STORE_TRUE, .default_value=false, .help="set the bar switch"}
    };
    argparser = cmn_argparser_init(argv[0], "An example program.");
    cmn_argparser_set_arguments(argparser, args, 3);
    option_map = cmn_argparser_parse(argparser, argc, argv);
    cmn_map_at(option_map, (void *) "arg", (void **) &result);
    cmn_argparser_destroy(argparser);
    return result;
}
