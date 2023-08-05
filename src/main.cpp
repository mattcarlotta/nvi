#include "arg.h"
#include "config.h"
#include "options.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    nvi::Arg arg(argc, argv);
    nvi::options_t options = arg.get_options();

    if (options.config.length()) {
        nvi::Config config(options.config);
        options = config.get_options();
    }

    nvi::Parser parser(options);

    parser.parse_envs()->check_envs()->set_or_print_envs();

    std::exit(0);
}
