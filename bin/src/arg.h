#ifndef NVI_ARG_H
#define NVI_ARG_H

#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
/** @class
 * Parses flags and arguments from `argv`.
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
 * ```
 */
class arg_parser {
    public:
    string config;
    bool debug = false;
    string command;
    vector<char *> commands;
    string dir;
    vector<string> files = vector<string>{".env"};
    vector<string> required_envs;
    string invalid_arg;
    string invalid_args;
    arg_parser(int &argc, char *argv[]);

    private:
    /**
     * Logs error/warning/debug arg_parser details.
     */
    void log(unsigned int code) const noexcept;
    /**
     * Parses flags that expect single arguments.
     *
     * @param `code` refers to the error/warning/debug message.
     *
     * @return a single argument as a string
     */
    string parse_single_arg(unsigned int code);
    /**
     * Parses flags that expect one or many arguments.
     *
     * @param `code` refers to the error/warning/debug message.
     *
     * @return a vector of string arguments.
     */
    vector<string> parse_multi_arg(unsigned int code);
    /**
     * Parses the `-e` or `--exec` flag arguments and assigns it to a public field `commands`.
     */
    void parse_command_args();

    private:
    size_t index;
    int argc;
    string bin_name;
    char **argv;
};
} // namespace nvi

#endif
