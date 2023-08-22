#include "arg.h"
#include "config.h"
#include "generator.h"
#include "lexer.h"
#include "options.h"
#include "parser.h"
#include <cstdlib>

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
    env_map_t env_map = parser.parse_tokens()->check_envs()->get_env_map();

    Generator generator(env_map, options);
    generator.set_or_print_envs();

    std::exit(EXIT_SUCCESS);
}
