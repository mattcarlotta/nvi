#include "generator.h"
#include "format.h"
#include "log.h"
#include "logger.h"
#include "options.h"
#include "parser.h"
#include <cerrno>
#include <iomanip>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

namespace nvi {
    inline constexpr char OPEN_BRACE = '{';  // 0x7b
    inline constexpr char CLOSE_BRACE = '}'; // 0x7d

    Generator::Generator(const env_map_t &&env_map, const options_t &&options)
        : _env_map(std::move(env_map)), _options(std::move(options)), logger(LOGGER::GENERATOR, _options) {}

    void Generator::set_or_print_envs() const noexcept {
        if (_options.commands.size()) {
            pid_t pid = fork();

            if (pid == 0) {
                for (const auto &[key, value] : _env_map) {
                    setenv(key.c_str(), value.c_str(), 0);
                    if (_options.debug) {
                        logger.log_debug(DEBUG_GENERATOR_KEYVALUE, fmt::format(R"(Set process key "%s" to value "%s".)",
                                                                               key.c_str(), value.c_str()));
                    }
                }

                // NOTE: Unfortunately, have to set ENVs to the parent process because
                // "execvpe" doesn't exist on POSIX, but also it doesn't allow binaries
                // that were called to locate other binaries found within shell "PATH"
                // for example: "npm run dev" won't work because "npm" can't find "node"
                execvp(_options.commands[0], _options.commands.data());
                if (errno == ENOENT) {
                    logger.debug(COMMAND_ENOENT);
                    _exit(-1);
                }
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                logger.fatal(COMMAND_FAILED_TO_RUN);
            }
        } else if (_options.print) {
            // print ENVs as a JSON formatted string to stdout
            const std::string last_key = std::prev(_env_map.end())->first;
            std::cout << OPEN_BRACE;
            for (auto const &[key, value] : _env_map) {
                std::cout << std::quoted(key) << ": " << std::quoted(value) << (key != last_key ? "," : "");
            }
            std::cout << CLOSE_BRACE << std::endl;
        } else if (not _options.save) {
            logger.fatal(NO_ACTION_ERROR);
        }
    }
} // namespace nvi
