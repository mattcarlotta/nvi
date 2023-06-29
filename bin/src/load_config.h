#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>
#include <optional>

namespace nvi {
class config {
    private:
    std::string env;
    std::ifstream env_config_file;
    std::string file_path;
    nlohmann::json::object_t env_config;

    public:
    config(const std::string &environment, const std::string dir = "");
    const std::optional<bool> get_debug() noexcept;
    const std::optional<std::string> get_dir() noexcept;
    const std::vector<std::string> get_files();
    const std::vector<std::string> get_required_envs() noexcept;
};
}; // namespace nvi

#endif
