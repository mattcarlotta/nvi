#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
class parser {
    public:
    std::map<string, string> env_map;
    parser(const vector<string> *files, const std::optional<string> &dir, const vector<string> *required_envs = nullptr,
           const bool &debug = false);
    void check_envs();
    parser *parse_envs() noexcept;
    void set_envs();
    void print_envs();

    private:
    void log(unsigned int code) const;
    parser *read(const string &env_file_name);
    parser *parse();

    private:
    const vector<string> *files;
    string dir;
    bool debug;
    const vector<string> *required_envs;
    std::ifstream env_file;
    string loaded_file;
    string file_name;
    string file_path;
    size_t file_length;
    size_t byte_count;
    size_t line_count;
    size_t val_byte_count;
    int assignment_index;
    string key;
    string key_prop;
    string value;
    vector<string> undefined_keys;
};
}; // namespace nvi

#endif
