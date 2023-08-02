#ifndef NVI_ARG_H
#define NVI_ARG_H

#include "log.h"
#include "options.h"
#include <string>
#include <vector>

namespace nvi {
    /**
     * @detail Parses flags and arguments from `argv` into `options`.
     * @param `argc` holds the number of arguments in `argv`.
     * @param `argv` contains flags and any arguments associated with them.
     * @see `nvi -h` or `nvi --help` for more information about the flags and arguments.
     * @example Parsing flags and arguments
     *
     * int argc = 5;
     * char *argv[] = {(char *)"nvi", (char *)"--config",  (char *)"test", (char *)"--debug", NULL };
     * nvi::Arg_Parser args(argc, argv);
     * nvi::options_t options = args.get_options();
     */
    class Arg_Parser {
        public:
            Arg_Parser(int &argc, char *argv[]);
            const options_t &get_options() const noexcept;

        private:
            options_t _options;
            int _index = 0;
            int _argc;
            std::string _bin_name;
            char **_argv;
            std::string _invalid_arg;
            std::string _invalid_args;
            std::string _command;

            std::string parse_single_arg(const messages_t &code) noexcept;
            std::vector<std::string> parse_multi_arg(const messages_t &code) noexcept;
            void parse_command_args() noexcept;
            void remove_invalid_arg() noexcept;
            void log(const messages_t &code) const noexcept;
    };
} // namespace nvi

#endif
