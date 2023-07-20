#include "arg.h"
#include "config.h"
#include "json.cpp"
#include "options.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    nvi::arg_parser arg(argc, argv);
    nvi::options options = arg.get_options();

    if (options.config.length()) {
        nvi::config config(options.config);
        options = config.get_options();
    }

    nvi::parser parser(options);

    parser.parse_envs()->check_envs()->set_envs();

    std::exit(0);
}
