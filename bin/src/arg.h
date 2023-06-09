#ifndef NVI_ARG_H
#define NVI_ARG_H

#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
class arg_parser {
    private:
    unsigned int key;
    int argc;
    char **argv;
    void log(unsigned int code) const;
    string parse_single_arg(unsigned int code);
    vector<string> parse_multi_arg(unsigned int code);

    public:
    string config;
    bool debug = false;
    string dir;
    vector<string> files = vector<string>{".env"};
    vector<string> required_envs;
    string invalid_arg;
    string invalid_args;
    arg_parser(int &argc, char *argv[]);
};
} // namespace nvi

#endif
