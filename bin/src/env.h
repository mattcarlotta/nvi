#ifndef NVI_ENV_H
#define NVI_ENV_H

#include "json.cpp"
#include <filesystem>
#include <fstream>

using std::string;

namespace nvi {
class file {
    private:
    bool show_log;
    std::ifstream env_file;
    string loaded_file;
    string file_name;
    std::filesystem::path file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;

    public:
    file(const string &dir, const string &env_file_name, const bool &debug);

    nlohmann::json parse(nlohmann::json env_map);
};
}; // namespace nvi

#endif
