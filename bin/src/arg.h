#ifndef NVI_ARG_H
#define NVI_ARG_H

#include <map>
#include <string>

using std::string;

namespace nvi {
class arg_parser {
    private:
    std::map<string, string> args;

    public:
    arg_parser(int &argc, char *argv[]);

    const string get(const string &flag);
};
} // namespace nvi

#endif
