#include "arg.h"
#include "env.h"
#include "json.cpp"
#include "load_config.h"

int main(int argc, char *argv[]) {
    nvi::arg_parser args(argc, argv);

    const auto env_config = args.get("--config");
    if (env_config.has_value()) {
        nvi::config config(env_config.value());
        nvi::file file(config.get_dir(), config.get_debug().value());

        for (const auto env : config.get_files()) {
            file.read(env)->parse();
        }

        file.check_required(config.get_required_envs())->print();
    } else {
        nvi::file file(args.get("--dir"), args.get("--debug") == "on");

        file.read(args.get("--file").value_or(".env"))->parse()->print();
    }

    return 0;
}
