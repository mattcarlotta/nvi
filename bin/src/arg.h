#ifndef NVI_ARG_H
#define NVI_ARG_H

#include <map>
#include <optional>
#include <string>
#include <vector>

using std::string;

namespace nvi {
class arg_parser {
    public:
    string config;
    std::vector<string> files = std::vector<string>{".env"};
    bool debug = false;
    string dir;
    std::vector<string> required_envs;
    arg_parser(int &argc, char *argv[]);

    private:
    void log(int code) const;
};
} // namespace nvi

#endif
