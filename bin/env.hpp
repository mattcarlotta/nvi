#include "json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using std::string;

class envfile {
    private:
    std::ifstream env_file;
    string file;
    const std::filesystem::path *file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;

    public:
    envfile(const std::filesystem::path &env_path);

    nlohmann::json parse(nlohmann::json env_map);
};
