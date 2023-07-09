#include "parser.h"
#include "constants.h"
#include "format.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

using std::string;
using std::vector;

namespace nvi {
parser::parser(const vector<string> *files, const std::optional<string> &dir, const vector<string> *required_envs,
               const bool &debug)
    : files(files), required_envs(required_envs), debug(debug), dir(dir.value_or("")),
      env_map(std::map<string, string>()) {}

parser *parser::parse() {
    while (this->byte_count < this->file_length) {
        const string line = this->loaded_file.substr(this->byte_count, this->file_length);
        const int line_delimiter_index = line.find(constants::LINE_DELIMITER);
        if (line[0] == constants::COMMENT || line[0] == constants::LINE_DELIMITER) {
            ++this->line_count;
            this->byte_count += line_delimiter_index + 1;
            continue;
        }

        this->assignment_index = line.find(constants::ASSIGN_OP);
        if (line_delimiter_index >= 0 && this->assignment_index >= 0) {
            this->key = line.substr(0, this->assignment_index);

            const string line_slice = line.substr(this->assignment_index + 1, line.length());
            this->val_byte_count = 0;
            this->value = "";
            while (this->val_byte_count < line_slice.length()) {
                const char current_char = line_slice[this->val_byte_count];
                const char next_char = line_slice[this->val_byte_count + 1];

                if (current_char == constants::LINE_DELIMITER) {
                    break;
                }

                if (current_char == constants::BACK_SLASH && next_char == constants::LINE_DELIMITER) {
                    ++this->line_count;
                    this->val_byte_count += 2;
                    continue;
                }

                if (current_char == constants::DOUBLE_QUOTE) {
                    this->value += constants::BACK_SLASH;
                }

                if (current_char == constants::DOLLAR_SIGN && next_char == constants::OPEN_BRACE) {
                    const string val_slice_str = line_slice.substr(this->val_byte_count, line_slice.length());

                    const int interp_close_index = val_slice_str.find(constants::CLOSE_BRACE);
                    if (interp_close_index >= 0) {
                        this->key_prop = val_slice_str.substr(2, interp_close_index - 2);

                        string interpolated_value = "";
                        if (this->env_map.count(this->key_prop)) {
                            interpolated_value = this->env_map.at(this->key_prop);
                        } else if (this->debug) {
                            this->log(constants::PARSER_INTERPOLATION_WARNING);
                        }

                        this->value += interpolated_value;

                        this->val_byte_count += this->key_prop.length() + 3;

                        continue;
                    } else {
                        this->log(constants::PARSER_INTERPOLATION_ERROR);
                        std::exit(1);
                    }
                }

                this->value += line_slice[this->val_byte_count];

                ++this->val_byte_count;
            }

            if (this->key.length()) {
                this->env_map[this->key] = this->value;
                if (this->debug) {
                    this->log(constants::PARSER_DEBUG);
                }
            }

            this->byte_count += this->assignment_index + this->val_byte_count + 1;
        } else {
            this->byte_count = this->file_length;
        }
    }

    if (this->debug) {
        this->log(constants::PARSER_DEBUG_FILE_PROCESSED);
    }

    this->env_file.close();

    return this;
}

int parser::print_envs() {
    if (this->required_envs != nullptr && this->required_envs->size()) {
        for (const string key : *this->required_envs) {
            if (!this->env_map.count(key)) {
                this->undefined_keys.push_back(key);
            }
        }

        if (this->undefined_keys.size()) {
            this->log(constants::PARSER_REQUIRED_ENV_ERROR);
            std::exit(1);
        }
    }

    if (!this->env_map.size()) {
        this->log(constants::PARSER_EMPTY_ENVS);
        std::exit(1);
    }

    const string last_key = std::prev(this->env_map.end())->first;
    std::cout << "{" << std::endl;
    for (auto const &[key, value] : this->env_map) {
        const string comma = key != last_key ? "," : "";
        std::cout << std::setw(4) << "\"" << key << "\""
                  << ": \"" << value << "\"" << comma << std::endl;
    }
    std::cout << "}" << std::endl;

    return 0;
}

parser *parser::read(const string &env_file_name) {
    this->file_path = string(std::filesystem::current_path() / this->dir / env_file_name);
    this->file_name = env_file_name;
    this->env_file = std::ifstream(this->file_path, std::ios_base::in);
    if (!this->env_file.good()) {
        this->log(constants::PARSER_FILE_ERROR);
        std::exit(1);
    }
    this->loaded_file = string{std::istreambuf_iterator<char>(this->env_file), std::istreambuf_iterator<char>()};
    this->file_length = this->loaded_file.length();
    if (!this->file_length) {
        this->log(constants::PARSER_EMPTY_ENVS);
        std::exit(1);
    }
    this->byte_count = 0;
    this->line_count = 0;

    return this;
}

parser *parser::parse_envs() noexcept {
    for (const string env : *this->files) {
        this->read(env)->parse();
    }

    return this;
}

void parser::log(unsigned int code) const {
    switch (code) {
    case constants::PARSER_INTERPOLATION_WARNING: {
        std::clog << format("[nvi] (parser::INTERPOLATION_WARNING::%s:%d:%d) The key \"%s\" contains an invalid "
                            "interpolated variable: \"%s\". Unable to locate a value that corresponds to this key.",
                            this->file_name.c_str(), this->line_count + 1,
                            this->assignment_index + this->val_byte_count + 2, this->key.c_str(),
                            this->key_prop.c_str())
                  << std::endl;
        break;
    }
    case constants::PARSER_INTERPOLATION_ERROR: {
        std::cerr << format("[nvi] (parser::INTERPOLATION_ERROR::%s:%d:%d) The key \"%s\" contains an "
                            "interpolated \"{\" operator, but appears to be missing a closing \"}\" operator.",
                            this->file_name.c_str(), this->line_count + 1,
                            this->assignment_index + this->val_byte_count + 2, this->key.c_str())
                  << std::endl;
        break;
    }
    case constants::PARSER_DEBUG: {
        std::clog << format("[nvi] (parser::DEBUG::%s:%d:%d) Set key \"%s\" to equal value \"%s\".",
                            this->file_name.c_str(), this->line_count + 1,
                            this->assignment_index + this->val_byte_count + 2, this->key.c_str(), this->value.c_str())
                  << std::endl;
        break;
    }
    case constants::PARSER_DEBUG_FILE_PROCESSED: {
        const string line_letter = this->line_count != 1 ? "s" : "";
        std::clog << format("[nvi] (parser::DEBUG_FILE_PROCESSED::%s) Processed %d line%s and %d bytes!\n",
                            this->file_name.c_str(), this->line_count, line_letter.c_str(), this->byte_count)
                  << std::endl;
        break;
    }
    case constants::PARSER_REQUIRED_ENV_ERROR: {
        std::stringstream envs;
        std::copy(this->undefined_keys.begin(), this->undefined_keys.end(), std::ostream_iterator<string>(envs, ","));

        std::cerr << format("[nvi] (parser::REQUIRED_ENV_ERROR) The following ENV keys are marked as required: \"%s\""
                            " but they are undefined after all of the .env files were parsed.",
                            envs.str().c_str())
                  << std::endl;
        break;
    }
    case constants::PARSER_FILE_ERROR: {
        std::cerr << format(
                         "[nvi] (parser::FILE_ERROR) Unable to locate \"%s\". The .env file doesn't appear to exist!",
                         this->file_path.c_str())
                  << std::endl;
        break;
    }
    case constants::PARSER_EMPTY_ENVS: {
        std::cerr << format("[nvi] (parser::PARSER_EMPTY_ENVS) Unable to parse any ENVS! Please ensure the \"%s\" file "
                            "is not empty.",
                            this->file_name.c_str())
                  << std::endl;
        break;
    }
    default:
        break;
    }
}
}; // namespace nvi
