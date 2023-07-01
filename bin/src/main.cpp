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
        dir = config.get_dir();
        debug = config.get_debug().value();
        files = config.get_files().value();
        required_envs = config.get_required_envs();
    }

    nvi::parser parser(dir, debug, required_envs);

    for (const string env : files) {
        parser.read(env)->parse();
    }

    parser.print();

    return 0;
}
