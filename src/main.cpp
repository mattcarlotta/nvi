#include "api.h"
#include "arg.h"
#include "config.h"
#include "generator.h"
#include "lexer.h"
#include "options.h"
#include "parser.h"
#include <cstdlib>
#include <string>

int main(int argc, char *argv[]) {
    using namespace nvi;

    Arg arg(argc, argv);
    options_t options = arg.get_options();

    if (options.config.length()) {
        Config config(options.config);
        options = config.get_options();
    }

    std::string api_envs;
    if (options.project.length()) {
        API api(options);
        api_envs = api.get_key_from_input()->fetch_envs();
    }

    Lexer lexer(options);
    tokens_t tokens =
        options.project.length() ? lexer.parse_api_response(api_envs)->get_tokens() : lexer.parse_files()->get_tokens();

    Parser parser(std::move(tokens), options);
    env_map_t env_map = parser.parse_tokens()->check_envs()->get_env_map();

    Generator generator(std::move(env_map), std::move(options));
    generator.set_or_print_envs();

    std::exit(EXIT_SUCCESS);
}
