#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <optional>

using std::string;

namespace nvi {
class parser {
    private:
    bool debug;
    std::ifstream env_file;
    string loaded_file;
    string file_name;
    string dir;
    std::filesystem::path file_path;
    unsigned int byte_count;
    unsigned int line_count;

    public:
    nlohmann::json::object_t env_map;
    parser(const std::optional<string> &dir, const bool debug = false) noexcept;
    parser *read(const string &env_file_name);
    parser *parse();
    parser *check_required(const std::vector<string> &required_envs);
    void print();
};
}; // namespace nvi

#endif
