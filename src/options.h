#ifndef NVI_OPTIONS_H
#define NVI_OPTIONS_H

#include <string>
#include <vector>

namespace nvi {
    struct Options {
            std::vector<char *> commands;
            std::string config;
            bool debug = false;
            std::string dir;
            std::vector<std::string> files{".env"};
            std::vector<std::string> required_envs;
    };

    typedef Options options_t;
}; // namespace nvi

#endif
