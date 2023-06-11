#ifndef ENV_H
#define ENV_H

#include "json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using std::string;

inline extern const char COMMENT = '#';
inline extern const char LINE_DELIMITER = '\n';
inline extern const char BACK_SLASH = '\\';
inline extern const char ASSIGN_OP = '=';
inline extern const char DOLLAR_SIGN = '$';
inline extern const char OPEN_BRACE = '{';
inline extern const char CLOSE_BRACE = '}';

class envfile {
    private:
    std::ifstream env_file;
    string file;
    string file_name;
    std::filesystem::path file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;

    public:
    envfile(const string &dir, const string &env_file_name);

    nlohmann::json parse(nlohmann::json env_map);
};
#endif
