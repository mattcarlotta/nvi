#include "generator.h"
#include "log.h"
#include "options.h"
#include "parser.h"
#include <cerrno>
#include <iomanip>
#include <sys/wait.h>
#include <unistd.h>

namespace nvi {
    Generator::Generator(const env_map_t &env_map, const options_t options) : _env_map(env_map), _options(options) {}

    void Generator::set_or_print_envs() const noexcept {
        if (_options.commands.size()) {
            pid_t pid = fork();

            if (pid == 0) {
                for (const auto &[key, value] : _env_map) {
                    setenv(key.c_str(), value.c_str(), 0);
                }

                // NOTE: Unfortunately, have to set ENVs to the parent process because
                // "execvpe" doesn't exist on POSIX, but also it doesn't allow binaries
                // that were called to locate other binaries found within shell "PATH"
                // for example: "npm run dev" won't work because "npm" can't find "node"
                execvp(_options.commands[0], _options.commands.data());
                if (errno == ENOENT) {
                    log(COMMAND_ENOENT_ERROR);
                    _exit(-1);
                }
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                log(COMMAND_FAILED_TO_RUN);
            }
        } else {
            // if a command wasn't provided, print ENVs as a JSON formatted string to stdout
            const std::string last_key = std::prev(_env_map.end())->first;
            std::cout << "{";
            for (auto const &[key, value] : _env_map) {
                std::cout << std::quoted(key) << ": " << std::quoted(value) << (key != last_key ? "," : "");
            }
            std::cout << "}" << std::endl;
        }
    }

    void Generator::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case COMMAND_ENOENT_ERROR: {
            NVI_LOG_DEBUG(
                COMMAND_ENOENT_ERROR,
                R"(The specified command encountered an error. The command "%s" doesn't appear to exist or may not reside in a directory within the shell PATH.)",
                _options.commands[0]);
            break;
        }
        case COMMAND_FAILED_TO_RUN: {
            NVI_LOG_ERROR_AND_EXIT(
                COMMAND_FAILED_TO_RUN,
                "Unable to run the specified command. See terminal logs for more information.",
                NULL);
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
