#include "arg.h"
#include "config.h"
#include "options.h"
// #include "parser.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
    using namespace nvi;

    Arg arg(argc, argv);
    options_t options = arg.get_options();

    Lexer lexer(options);
    tokens_t tokens = lexer.read_files()->get_tokens();

    for (const auto &token : tokens) {
        std::clog << '\n';
        std::clog << "FILE: " << token.file << '\n';
        std::clog << "KEY: " << token.key.value() << '\n';
        std::clog << "VALUE(" << token.values.size() << "): ";
        for (const auto &vt : token.values) {
            std::clog << (vt.value.has_value() ? vt.value.value() : "") << "(" << static_cast<int>(vt.type) << ")";
        }
        std::clog << std::endl;
    }
    // if (options.config.length()) {
    //     Config config(options.config);
    //     options = config.get_options();
    // }

    // Parser parser(options);

    // parser.parse_envs()->check_envs()->set_or_print_envs();

    std::exit(0);
}
