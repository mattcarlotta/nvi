#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>
#include <optional>
#include <string>

using std::string;

namespace nvi {
class config {
    private:
    string env;
    std::ifstream env_config_file;
    string file_path;
    nlohmann::json::object_t env_config;

    public:
    config(const string &environment, const string dir = "");
    const std::optional<bool> get_debug() noexcept;
    const std::optional<string> get_dir() noexcept;
    const std::vector<string> get_files();
    const std::vector<string> get_required_envs() noexcept;
};
}; // namespace nvi

#endif
