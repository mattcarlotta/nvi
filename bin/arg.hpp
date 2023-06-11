#ifndef ARG_H
#define ARG_H

#include <map>
#include <string>

class argparser {
    private:
    std::map<std::string, std::string> args;

    public:
    argparser(int &argc, char *argv[]);

    const std::string get(const std::string &flag);
};
#endif
