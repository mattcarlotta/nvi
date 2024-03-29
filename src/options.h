#ifndef NVI_OPTIONS_H
#define NVI_OPTIONS_H

#include <string>
#include <vector>

namespace nvi {
    typedef struct Options {
        bool api = false;
        std::vector<char *> commands;
        std::string config;
        bool debug = false;
        std::string dir;
        std::string environment;
        std::vector<std::string> files{".env"};
        bool print = false;
        std::string project;
        std::vector<std::string> required_envs;
        bool save = false;
    } options_t;
}; // namespace nvi

#endif
