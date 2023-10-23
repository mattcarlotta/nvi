#include "api.h"
#include "arg.h"
#include "config.h"
#include "generator.h"
#include "lexer.h"
#include "options.h"
#include "parser.h"
#include <cstdlib>
#include <string>
#include <utility>

int main(int argc, char *argv[]) {
    nvi::options_t options;
    std::string api_envs;

    nvi::Arg arg(argc, argv, options);

    if (options.config.length()) {
        nvi::Config config(options);
        config.generate_options();
    }

    if (options.api) {
        nvi::Api api(options);
        api_envs = api.get_key_from_file_or_input()->fetch_envs()->get_envs();
    }

    nvi::Lexer lexer(options);
    nvi::tokens_t tokens =
        options.api ? lexer.parse_api_response(api_envs)->get_tokens() : lexer.parse_files()->get_tokens();

    nvi::Parser parser(std::move(tokens), options);
    nvi::env_map_t env_map = parser.parse_tokens()->get_env_map();

    nvi::Generator generator(std::move(env_map), std::move(options));
    generator.set_or_print_envs();

    std::exit(EXIT_SUCCESS);
}
