#include "arg.h"
#include "json.cpp"
#include "load_config.h"
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
        files = config.get_files();
        required_envs = config.get_required_envs();
    }

    nvi::parser parser(dir, debug);

    for (const auto env : files) {
        parser.read(env)->parse();
    }

    parser.check_required(required_envs)->print();

    return 0;
}
