#include "argparser.h"
#include "map.h"
#include "ftc.h"
#include "utilities.h"
#include "try.h"

extern int main(int argc, const char* argv[]) {
	char* url;
	cmn_argparser_t argparser;
	cmn_map_t option_map;
    struct cmn_argparser_argument args[] = {
        { .name = "url",	.help="the file transfer server URL address"},
    };
    argparser = cmn_argparser_init(argv[0], "File transfer client.");
    cmn_argparser_set_arguments(argparser, args, 1);
    option_map = cmn_argparser_parse(argparser, argc, argv);
	cmn_map_at(option_map, (void*)"url", (void**)&url);
    cmn_argparser_destroy(argparser);
	try(ftc_start(url), 1);
	return 0;
}
