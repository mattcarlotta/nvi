#include "arg.h"
#include "config.h"
#include "format.h"
#include "json.cpp"
#include "parser.h"
#include <cerrno>
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
    vector<char *> commands = arg.commands;
    std::optional<string> dir = arg.dir;
    bool debug = arg.debug;
    vector<string> files = arg.files;
    vector<string> required_envs = arg.required_envs;

    if (arg.config.length()) {
        nvi::config config(&arg.config);
        commands = config.commands;
        dir = config.dir;
        debug = config.debug;
        files = config.files;
        required_envs = config.required_envs;
    }

    nvi::parser parser(&files, dir, &required_envs, debug);

    parser.parse_envs()->check_envs();

    if (commands.size()) {
        pid_t pid = fork();

        if (pid == 0) {
            // NOTE: Unfortunately, have to set ENVs to the parent process because
            // "execvpe" doesn't exist on POSIX and "execle" doesn't allow binaries
            // that were called to locate other binaries found within shell "PATH"
            // for example: "npm run dev" won't work because "npm" can't find "node"
            parser.set_envs();

            execvp(commands[0], commands.data());
            if (errno == ENOENT) {
                std::cerr
                    << nvi::format(
                           "[nvi] (main::COMMAND_ENOENT_ERROR) The specified command encountered an error. The command "
                           "\"%s\" doesn't appear to exist or may not reside in a directory within the shell PATH.",
                           commands[0])
                    << std::endl;
                _exit(-1);
            }
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
