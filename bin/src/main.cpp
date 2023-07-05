#include "arg.h"
#include "config.h"
#include "json.cpp"
#include "parser.h"
#include <optional>
#include <string>
#include <vector>

using std::string;
using std::vector;

int main(int argc, char *argv[]) {
    nvi::arg_parser arg(argc, argv);
    vector<string> files = arg.files;
    std::optional<string> dir = arg.dir;
    bool debug = arg.debug;
    vector<string> required_envs = arg.required_envs;

    if (arg.config.length()) {
        nvi::config config(arg.config);
        dir = config.dir;
        debug = config.debug;
        files = config.files;
        required_envs = config.required_envs;
    }

    nvi::parser parser(files, dir, debug, required_envs);

    return parser.parse_envs()->print_envs();
}
