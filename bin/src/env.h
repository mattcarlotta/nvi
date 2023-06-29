#ifndef NVI_ENV_H
#define NVI_ENV_H

#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <optional>

using std::string;

namespace nvi {
class file {
    private:
    bool debug;
    std::ifstream env_file;
    string loaded_file;
    string file_name;
    string dir;
    std::filesystem::path file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;

    public:
    nlohmann::json::object_t env_map;
    file(const std::optional<string> &dir, const bool debug = false);
    file *read(const string &env_file_name);
    file *parse();
    file *check_required(const std::vector<string> &required_envs);
    void print();
};
}; // namespace nvi

#endif
