#include "arg.h"
#include "json.cpp"
#include "load_config.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    nvi::arg_parser arg(argc, argv);

    const auto env_config = arg.config;
    if (env_config.length()) {
        nvi::config config(env_config);
        nvi::parser parser(config.get_dir(), config.get_debug().value());

        for (const auto env : config.get_files()) {
            parser.read(env)->parse();
        }

        parser.check_required(config.get_required_envs())->print();
    } else {
        nvi::parser parser(arg.dir, arg.debug);

        for (const auto env : arg.files) {
            parser.read(env)->parse();
        }

        parser.check_required(arg.required_envs)->print();
    }

    return 0;
}
