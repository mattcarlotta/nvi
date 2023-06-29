#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>

namespace nvi {
class config {
    private:
    std::string env;
    std::ifstream env_config_file;
    std::string file_path;
    nlohmann::json::object_t env_config;

    public:
    config(const std::string &environment, const std::string dir = "");
    const bool get_debug();
    const std::string get_dir();
    const std::vector<std::string> get_files();
    const std::vector<std::string> get_required_envs();
};
}; // namespace nvi

#endif
