#include "arg.h"
#include "env.h"
#include "json.cpp"
#include "load_config.h"

int main(int argc, char *argv[]) {
    nvi::arg_parser args(argc, argv);

    const string env_config = args.get("--config");
    if (env_config.length()) {
        nvi::config config(env_config);
        nvi::file file(config.get_dir(), config.get_debug());

        for (const string env : config.get_files()) {
            file.read(env)->parse();
        }

        file.check_required(config.get_required_envs())->print();
    } else {
        string env_file = args.get("--file");
        if (!env_file.length()) {
            env_file = ".env";
        }

        nvi::file file(args.get("--dir"), args.get("--debug") == "on");
        file.read(env_file)->parse()->print();
    }

    return 0;
}
