#include "json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using std::string;

const char COMMENT = '#';
const char LINE_DELIMITER = '\n';
const char BACK_SLASH = '\\';
const char ASSIGN_OP = '=';
const char DOLLAR_SIGN = '$';
const char OPEN_BRACE = '{';
const char CLOSE_BRACE = '}';

namespace Env {
class FileParser {
    private:
    std::ifstream env_file;
    string file;
    const std::filesystem::path *file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;

    public:
    FileParser(const std::filesystem::path &env_path) {
        file_path = &env_path;
        env_file = std::ifstream(file_path->string(), std::ios_base::in);
        if (!env_file.good()) {
            std::cerr << "Unable to locate: '" << file_path->string() << "'. The file doesn't appear to exist!"
                      << std::endl;
            exit(1);
        }
        file = string{std::istreambuf_iterator<char>(env_file), std::istreambuf_iterator<char>()};
    }

    nlohmann::json parse(nlohmann::json env_map) {
        while (byte_count < file.length()) {
            const string line = file.substr(byte_count, file.length());
            const int line_delimiter_index = line.find(LINE_DELIMITER);
            if (line[0] == COMMENT || line[0] == LINE_DELIMITER) {
                ++line_count;
                byte_count += line_delimiter_index + 1;
                continue;
            }

            const int assignment_index = line.find(ASSIGN_OP);
            if (line_delimiter_index >= 0 && assignment_index >= 0) {
                const string key = line.substr(0, assignment_index);

                const string line_slice = line.substr(assignment_index + 1, line.length());
                int val_byte_count = 0;
                string value = "";
                while (val_byte_count < line_slice.length()) {
                    const char current_char = line_slice[val_byte_count];
                    const char next_char = line_slice[val_byte_count + 1];

                    if (current_char == LINE_DELIMITER) {
                        break;
                    }

                    if (current_char == BACK_SLASH && next_char == LINE_DELIMITER) {
                        ++line_count;
                        val_byte_count += 2;
                        continue;
                    }

                    if (current_char == DOLLAR_SIGN && next_char == OPEN_BRACE) {
                        const string val_slice_str = line_slice.substr(val_byte_count, line_slice.length());

                        const int interp_close_index = val_slice_str.find(CLOSE_BRACE);
                        if (interp_close_index >= 0) {
                            const string key_prop = val_slice_str.substr(2, interp_close_index - 2);

                            string interpolated_value;
                            if (env_map.contains(key_prop)) {
                                interpolated_value = env_map.at(key_prop);
                            } else {
                                interpolated_value = "";
                                std::cerr << "The key '" << key << "' contains an invalid interpolated variable: '"
                                          << key_prop << "'. Unable to locate a value that corresponds to this key."
                                          << std::endl;
                            }

                            value += interpolated_value;

                            val_byte_count += key_prop.length() + 3;

                            continue;
                        } else {
                            ++line_count;
                            std::cerr
                                << "The key '" << key
                                << "' contains an interpolated variable: '${' operator but appears to be missing a "
                                   "closing '}' operator."
                                << std::endl;
                            exit(1);
                        }
                    }

                    value += line_slice[val_byte_count];

                    ++val_byte_count;
                }

                if (key.length()) {
                    env_map[key] = value;
                }

                byte_count += assignment_index + val_byte_count + 1;
            } else {
                byte_count = file.length();
            }
        }

        env_file.close();

        return env_map;
    }
};
}; // namespace Env
