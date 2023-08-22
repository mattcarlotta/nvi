#include "arg.h"
#include "config.h"
#include "lexer.h"
#include "options.h"
#include "parser.h"
#include <cstdlib>

std::string lookup_type(int code) {
    switch (code) {
    case 0:
        return "normal";
    case 1:
        return "interpolated";
    default:
        return "multiline";
    }
}

int main(int argc, char *argv[]) {
    using namespace nvi;

    Arg arg(argc, argv);
    options_t options = arg.get_options();

    if (options.config.length()) {
        Config config(options.config);
        options = config.get_options();
    }

    Lexer lexer(options);
    tokens_t tokens = lexer.read_files()->get_tokens();

    Parser parser(tokens, options);
    parser.parse_tokens()->check_envs()->set_or_print_envs();

    std::exit(EXIT_SUCCESS);
}
