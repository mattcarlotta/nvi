#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
class config {
    private:
    const string *env;
    string file_path;
    nlohmann::json parsed_config;
    nlohmann::json::object_t env_config;
    void log(unsigned int code) const;

    public:
    bool debug = false;
    string dir;
    vector<string> files;
    vector<string> required_envs = vector<string>();
    config(const string *environment, const string env_dir = "");
};
}; // namespace nvi

#endif
