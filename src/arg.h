#ifndef NVI_ARG_H
#define NVI_ARG_H

#include "log.h"
#include "options.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nvi {
    enum class FLAG {
        API,
        CONFIG,
        DEBUG,
        DIRECTORY,
        ENVIRONMENT,
        EXECUTE,
        FILES,
        HELP,
        PRINT,
        PROJECT,
        REQUIRED,
        SAVE,
        VERSION,
        UNKNOWN
    };

    /**
     * @detail Parses flags and arguments from `argv` into `options`.
     * @param `argc` holds the number of arguments in `argv`.
     * @param `argv` contains flags and any arguments associated with them.
     * @see `nvi --help` for more information about the flags and arguments.
     * @example Parsing flags and arguments
     *
     * int argc = 5;
     * char *argv[] = {(char *)"nvi", (char *)"--config",  (char *)"test", (char *)"--debug", (char *)NULL };
     * nvi::options_t options;
     * nvi::Arg args(argc, argv, &options);
     */
    class Arg {
        public:
        Arg(int &argc, char *argv[], options_t *options);

        private:
        int _argc;
        char **_argv;
        options_t *_options;
        int _index = 0;
        std::string _bin_name;
        std::string _invalid_flag;
        std::string _invalid_args;
        std::string _command;

        std::unordered_map<std::string_view, FLAG> FLAGS{
            {"--api", FLAG::API},
            {"--config", FLAG::CONFIG},
            {"--debug", FLAG::DEBUG},
            {"--directory", FLAG::DIRECTORY},
            {"--environment", FLAG::ENVIRONMENT},
            {"--", FLAG::EXECUTE},
            {"--files", FLAG::FILES},
            {"--help", FLAG::HELP},
            {"--print", FLAG::PRINT},
            {"--project", FLAG::PROJECT},
            {"--required", FLAG::REQUIRED},
            {"--save", FLAG::SAVE},
            {"--version", FLAG::VERSION},
        };
        std::string parse_single_arg(const messages_t &code) noexcept;
        std::vector<std::string> parse_multi_arg(const messages_t &code) noexcept;
        void parse_command_args() noexcept;
        void remove_invalid_flag() noexcept;
        void log(const messages_t &code) const noexcept;
    };
} // namespace nvi

#endif
