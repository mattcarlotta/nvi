#include "arg.h"
#include "json.cpp"
#include "load_config.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    nvi::arg_parser args(argc, argv);

    const auto env_config = args.get("--config");
    if (env_config.has_value()) {
        nvi::config config(env_config.value());
        nvi::parser parser(config.get_dir(), config.get_debug().value());

        for (const auto env : config.get_files()) {
            parser.read(env)->parse();
        }

        parser.check_required(config.get_required_envs())->print();
    } else {
        nvi::parser parser(args.get("--dir"), args.get("--debug") == "on");

        parser.read(args.get("--file").value_or(".env"))->parse()->print();
    }

    return 0;
}
