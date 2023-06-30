#ifndef NVI_ARG_H
#define NVI_ARG_H

#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
class arg_parser {
    public:
    string config;
    bool debug = false;
    string dir;
    vector<string> files = vector<string>{".env"};
    vector<string> required_envs;
    arg_parser(int &argc, char *argv[]);

    private:
    void log(int code) const;
};
} // namespace nvi

#endif
