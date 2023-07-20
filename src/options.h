#ifndef NVI_OPTIONS_H
#define NVI_OPTIONS_H

#include <string>
#include <vector>

namespace nvi {
    struct options {
            std::vector<char *> commands;
            std::string config;
            bool debug = false;
            std::string dir;
            std::vector<std::string> files = std::vector<std::string>{".env"};
            std::vector<std::string> required_envs;
    };
}; // namespace nvi

#endif