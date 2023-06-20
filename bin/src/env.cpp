#include "env.h"
#include "constants.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using std::string;

namespace nvi {
file::file(const string &dir, const string &env_file_name, const bool &debug) {
    show_log = debug;
    file_path = std::filesystem::current_path() / dir / env_file_name;
    file_name = env_file_name;
    env_file = std::ifstream(file_path.string(), std::ios_base::in);
    if (!env_file.good()) {
        std::cerr << "[nvi] ERROR: Unable to locate '" << file_path.string() << "'. The file doesn't appear to exist!"
                  << std::endl;
        exit(1);
    }
    loaded_file = string{std::istreambuf_iterator<char>(env_file), std::istreambuf_iterator<char>()};
}

nlohmann::json file::parse(nlohmann::json env_map) {
    while (byte_count < loaded_file.length()) {
        const string line = loaded_file.substr(byte_count, loaded_file.length());
        const int line_delimiter_index = line.find(constants::LINE_DELIMITER);
        if (line[0] == constants::COMMENT || line[0] == constants::LINE_DELIMITER) {
            ++line_count;
            byte_count += line_delimiter_index + 1;
            continue;
        }

        const int assignment_index = line.find(constants::ASSIGN_OP);
        if (line_delimiter_index >= 0 && assignment_index >= 0) {
            const string key = line.substr(0, assignment_index);

            const string line_slice = line.substr(assignment_index + 1, line.length());
            int val_byte_count = 0;
            string value = "";
            while (val_byte_count < line_slice.length()) {
                const char current_char = line_slice[val_byte_count];
                const char next_char = line_slice[val_byte_count + 1];

                if (current_char == constants::LINE_DELIMITER) {
                    break;
                }

                if (current_char == constants::BACK_SLASH && next_char == constants::LINE_DELIMITER) {
                    ++line_count;
                    val_byte_count += 2;
                    continue;
                }

                if (current_char == constants::DOLLAR_SIGN && next_char == constants::OPEN_BRACE) {
                    const string val_slice_str = line_slice.substr(val_byte_count, line_slice.length());

                    const int interp_close_index = val_slice_str.find(constants::CLOSE_BRACE);
                    if (interp_close_index >= 0) {
                        const string key_prop = val_slice_str.substr(2, interp_close_index - 2);

                        string interpolated_value = "";
                        if (env_map.contains(key_prop)) {
                            interpolated_value = env_map.at(key_prop);
                        } else {
                            std::clog << "[nvi]"
                                      << " (" << file_name << ":" << line_count + 1 << ":"
                                      << byte_count + assignment_index + val_byte_count + 2 << ") "
                                      << "The key '" << key << "' contains an invalid interpolated variable: '"
                                      << key_prop << "'. Unable to locate a value that corresponds to this key."
                                      << std::endl;
                        }

                        value += interpolated_value;

                        val_byte_count += key_prop.length() + 3;

                        continue;
                    } else {
                        std::cerr << "[nvi]"
                                  << " (" << file_name << ":" << line_count + 2 << ":"
                                  << assignment_index + val_byte_count + 2 << ") "
                                  << "ERROR: The key '" << key
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
                if (show_log) {
                    std::clog << "[nvi] (" << file_name << ":" << line_count + 1 << ":"
                              << byte_count + assignment_index + val_byte_count + 1 << ") Set key '" << key
                              << "' to equal value '" << value << "'." << std::endl;
                }
            }

            byte_count += assignment_index + val_byte_count + 1;
        } else {
            byte_count = loaded_file.length();
        }
    }

    if (show_log) {
        std::clog << "[nvi] (" << file_name << ") Processed " << line_count << " lines and " << byte_count << " bytes!"
                  << std::endl;
    }

    env_file.close();

    return env_map;
}
}; // namespace nvi
