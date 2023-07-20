#ifndef NVI_ARG_H
#define NVI_ARG_H

#include "options.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace nvi {
    /** @class
     * Parses flags and arguments from `argv` into `options`.
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
     * nvi::arg_parser args(argc, argv);
     *
     * nvi::options options = args.get_options();
     *
     * ```
     */
    class arg_parser {
        public:
            arg_parser(int &argc, char *argv[]);
            const options &get_options() const noexcept;

        private:
            options options_;
            int index_;
            int argc_;
            std::string bin_name_;
            char **argv_;
            std::string invalid_arg_;
            std::string invalid_args_;
            std::string command_;

            void log(uint8_t code) const noexcept;
            std::string parse_single_arg(unsigned int code);
            std::vector<std::string> parse_multi_arg(unsigned int code);
            void parse_command_args();
    };
} // namespace nvi

#endif
