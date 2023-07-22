#include "arg.h"
#include "config.h"
#include "options.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    nvi::Arg_Parser arg(argc, argv);
    nvi::Options options = arg.get_options();

    if (options.config.length()) {
        nvi::Config config(options.config);
        options = config.get_options();
    }

    nvi::Parser parser(options);

    parser.parse_envs()->check_envs()->set_envs();

    std::exit(0);
}
