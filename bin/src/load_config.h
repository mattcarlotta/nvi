#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>
#include <string>
#include <vector>

using std::string;

namespace nvi {
class config {
    private:
    string env;
    std::ifstream env_config_file;
    std::filesystem::path file_path;
    nlohmann::json env_config;

    public:
    config(const string &environment, const string dir = "");

    const bool get_debug();

    const string get_dir();

    const std::vector<string> get_files();

    const std::vector<string> get_required_envs();
};
}; // namespace nvi

#endif
