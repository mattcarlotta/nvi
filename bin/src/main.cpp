#include "arg.h"
#include "config.h"
#include "json.cpp"
#include "parser.h"
#include <iostream>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using std::string;
using std::vector;

int main(int argc, char *argv[]) {
    nvi::arg_parser arg(argc, argv);
    vector<string> files = arg.files;
    std::optional<string> dir = arg.dir;
    bool debug = arg.debug;
    vector<string> required_envs = arg.required_envs;

    if (arg.config.length()) {
        nvi::config config(&arg.config);
        dir = config.dir;
        debug = config.debug;
        files = config.files;
        required_envs = config.required_envs;
    }

    nvi::parser parser(&files, dir, &required_envs, debug);

    parser.parse_envs()->check_envs();

    if (arg.commands.size()) {
        pid_t pid = fork();

        if (pid == 0) {
            // NOTE: Unfortunately, have to set ENVs to the parent process because
            // "execvpe" doesn't exist on POSIX and "execle" doesn't allow binaries
            // that were called to locate other binaries found within shell "PATH"
            // for example: "npm run dev" won't work because "npm" can't find "node"
            parser.set_envs();

            execvp(arg.commands[0], arg.commands.data());
        } else if (pid > 0) {
            int status;
            wait(&status);
        } else {
            std::cerr << "[nvi] (main::COMMAND_FAILED_TO_START) Unable to run the specified command." << std::endl;
            std::exit(1);
        }
    } else {
        parser.print_envs();
    }
    return 0;
}
