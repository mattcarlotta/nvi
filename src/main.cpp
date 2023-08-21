#include "arg.h"
#include "config.h"
#include "options.h"
// #include "parser.h"
#include "lexer.h"

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

    Lexer lexer(options);
    tokens_t tokens = lexer.read_files()->get_tokens();

    for (const auto &token : tokens) {
        std::clog << '\n';
        std::clog << "FILE: " << token.file << '\n';
        std::clog << "KEY: " << token.key.value() << '\n';
        std::clog << "VALUE(" << token.values.size() << "): \n";
        for (const auto &vt : token.values) {
            std::clog << '\n';
            std::clog << "      VALUE_TYPE: " << lookup_type(static_cast<int>(vt.type)) << '\n';
            std::clog << "      VALUE: " << (vt.value.has_value() ? vt.value.value() : "") << '\n';
            std::clog << "      LINE: " << vt.line << '\n';
            std::clog << "      BYTE: " << vt.byte << '\n';
        }
    }
    std::clog << std::endl;
    // if (options.config.length()) {
    //     Config config(options.config);
    //     options = config.get_options();
    // }

    // Parser parser(options);

    // parser.parse_envs()->check_envs()->set_or_print_envs();

    std::exit(0);
}
