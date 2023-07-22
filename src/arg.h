#ifndef NVI_ARG_H
#define NVI_ARG_H

#include "options.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace nvi {
    /**
     * @detail Parses flags and arguments from `argv` into `options`.
     *
     * @param `argc` holds the number of arguments in `argv`.
     *
     * @param `argv` contains flags and any arguments associated with them.
     *
     * @see `nvi -h` or `nvi --help` for more information about the flags and arguments.
     *
     * @example Parsing flags and arguments
     * ```
     *
     * int argc = 5;
     *
     * char *argv[] = {(char *)"nvi", (char *)"--config",  (char *)"test", (char *)"--debug", NULL };
     *
     * nvi::Arg_Parser args(argc, argv);
     *
     * nvi::Options options = args.get_options();
     *
     * ```
     */
    class Arg_Parser {
        public:
            Arg_Parser(int &argc, char *argv[]);
            const Options &get_options() const noexcept;

        private:
            Options _options;
            int _index;
            int _argc;
            std::string _bin_name;
            char **_argv;
            std::string _invalid_arg;
            std::string _invalid_args;
            std::string _command;

            void log(uint8_t code) const noexcept;
            std::string parse_single_arg(unsigned int code);
            std::vector<std::string> parse_multi_arg(unsigned int code);
            void parse_command_args();
    };
} // namespace nvi

#endif
