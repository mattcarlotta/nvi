#include "parser.h"
#include "constants.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>

using std::string;
using std::vector;

namespace nvi {
parser::parser(const std::optional<string> &dir, const bool debug) noexcept {
    this->dir = dir.value_or("");
    this->debug = debug;
    this->env_map = nlohmann::json::object();
}

parser *parser::parse() {
    const unsigned int file_length = this->loaded_file.length();
    while (this->byte_count < file_length) {
        const string line = this->loaded_file.substr(this->byte_count, file_length);
        const int line_delimiter_index = line.find(constants::LINE_DELIMITER);
        if (line[0] == constants::COMMENT || line[0] == constants::LINE_DELIMITER) {
            ++this->line_count;
            this->byte_count += line_delimiter_index + 1;
            continue;
        }

        const int assignment_index = line.find(constants::ASSIGN_OP);
        if (line_delimiter_index >= 0 && assignment_index >= 0) {
            const string key = line.substr(0, assignment_index);

            const string line_slice = line.substr(assignment_index + 1, line.length());
            unsigned int val_byte_count = 0;
            string value = "";
            while (val_byte_count < line_slice.length()) {
                const char current_char = line_slice[val_byte_count];
                const char next_char = line_slice[val_byte_count + 1];

                if (current_char == constants::LINE_DELIMITER) {
                    break;
                }

                if (current_char == constants::BACK_SLASH && next_char == constants::LINE_DELIMITER) {
                    ++this->line_count;
                    val_byte_count += 2;
                    continue;
                }

                if (current_char == constants::DOLLAR_SIGN && next_char == constants::OPEN_BRACE) {
                    const string val_slice_str = line_slice.substr(val_byte_count, line_slice.length());

                    const int interp_close_index = val_slice_str.find(constants::CLOSE_BRACE);
                    if (interp_close_index >= 0) {
                        const string key_prop = val_slice_str.substr(2, interp_close_index - 2);

                        string interpolated_value = "";
                        if (this->env_map.count(key_prop)) {
                            interpolated_value = this->env_map.at(key_prop);
                        } else {
                            std::clog << "[nvi]"
                                      << " (" << this->file_name << ":" << this->line_count + 1 << ":"
                                      << assignment_index + val_byte_count + 2 << ") "
                                      << "WARNING: The key '" << key << "' contains an invalid interpolated variable: '"
                                      << key_prop << "'. Unable to locate a value that corresponds to this key."
                                      << std::endl;
                        }

                        value += interpolated_value;

                        val_byte_count += key_prop.length() + 3;

                        continue;
                    } else {
                        std::cerr << "[nvi]"
                                  << " (" << this->file_name << ":" << this->line_count + 2 << ":"
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
                this->env_map[key] = value;
                if (this->debug) {
                    std::clog << "[nvi] (" << this->file_name << ":" << this->line_count + 1 << ":"
                              << assignment_index + val_byte_count + 1 << ") DEBUG: Set key '" << key
                              << "' to equal value '" << value << "'." << std::endl;
                }
            }

            this->byte_count += assignment_index + val_byte_count + 1;
        } else {
            this->byte_count = file_length;
        }
    }

    if (this->debug) {
        const char conditional_plural_letter = this->line_count > 1 ? 's' : '\0';
        std::clog << "[nvi] (" << this->file_name << ") DEBUG: Processed " << this->line_count << " line"
                  << conditional_plural_letter << " and " << this->byte_count << " bytes!\n"
                  << std::endl;
    }

    this->env_file.close();

    return this;
}

void parser::print() { std::cout << std::setw(4) << this->env_map << std::endl; }

parser *parser::read(const string &env_file_name) {
    this->file_path = string(std::filesystem::current_path() / this->dir / env_file_name);
    this->file_name = env_file_name;
    this->env_file = std::ifstream(this->file_path, std::ios_base::in);
    if (!this->env_file.good()) {
        std::cerr << "[nvi] ERROR: Unable to locate '" << this->file_path << "'. The file doesn't appear to exist!"
                  << std::endl;
        exit(1);
    }
    this->loaded_file = string{std::istreambuf_iterator<char>(this->env_file), std::istreambuf_iterator<char>()};
    this->byte_count = 0;
    this->line_count = 0;

    return this;
}

parser *parser::check_required(const vector<string> &required_envs) {
    if (required_envs.size()) {
        vector<string> undefined_keys;

        for (const string key : required_envs) {
            if (!this->env_map.count(key)) {
                undefined_keys.push_back(key);
            }
        }

        if (undefined_keys.size()) {
            std::stringstream envs;
            std::copy(undefined_keys.begin(), undefined_keys.end(), std::ostream_iterator<string>(envs, ", "));
            std::cerr << "[nvi] ERROR: The following Envs are marked as required: " << envs.str()
                      << "but they are undefined after all of the .env files were parsed." << std::endl;
            exit(1);
        }
    }

    return this;
}
}; // namespace nvi
