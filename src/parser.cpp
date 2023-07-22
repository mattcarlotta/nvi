#include "parser.h"
#include "constants.h"
#include "format.h"
#include "json.cpp"
#include "options.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace nvi {
    parser::parser(const options &options) : options_(options) {}

    const std::map<std::string, std::string> &parser::get_env_map() const { return env_map_; }

    parser *parser::parse() {
        while (byte_count_ < file_.length()) {
            const std::string_view line = file_view_.substr(byte_count_, file_.length());
            const int line_delimiter_index = line.find(constants::LINE_DELIMITER);

            // skip lines that begin with comments
            if (line[0] == constants::COMMENT || line[0] == constants::LINE_DELIMITER) {
                ++line_count_;
                byte_count_ += line_delimiter_index + 1;
                continue;
            }

            // split key from value by assignment "="
            assignment_index_ = line.find(constants::ASSIGN_OP);
            if (line_delimiter_index >= 0 && assignment_index_ >= 0) {
                key_ = line.substr(0, assignment_index_);

                const std::string_view line_slice = line.substr(assignment_index_ + 1, line.length());
                val_byte_count_ = 0;
                value_ = "";
                while (val_byte_count_ < line_slice.length()) {
                    const char current_char = line_slice[val_byte_count_];
                    const char next_char = line_slice[val_byte_count_ + 1];

                    // stop parsing if there's a new line character
                    if (current_char == constants::LINE_DELIMITER) {
                        break;
                    }

                    // skip parsing escaped multi-line characters
                    if (current_char == constants::BACK_SLASH && next_char == constants::LINE_DELIMITER) {
                        ++line_count_;
                        val_byte_count_ += 2;
                        continue;
                    }

                    // try interpolating key into a value
                    if (current_char == constants::DOLLAR_SIGN && next_char == constants::OPEN_BRACE) {
                        const std::string_view val_slice_str = line_slice.substr(val_byte_count_, line_slice.length());

                        const int interp_close_index = val_slice_str.find(constants::CLOSE_BRACE);
                        if (interp_close_index >= 0) {
                            key_prop_ = val_slice_str.substr(2, interp_close_index - 2);
                            const char *val_from_proc = std::getenv(key_prop_.c_str());

                            // interpolate the value from env first, otherwise fallback to the env_map
                            std::string interpolated_value = "";
                            if (val_from_proc != nullptr && *val_from_proc != constants::NULL_TERMINATOR) {
                                interpolated_value = val_from_proc;
                            } else if (env_map_.count(key_prop_)) {
                                interpolated_value = env_map_.at(key_prop_);
                            } else if (options_.debug) {
                                log(constants::PARSER_INTERPOLATION_WARNING);
                            }

                            value_ += interpolated_value;

                            // factor in the interpolated key length with the "$", "{" and "}" character bytes
                            val_byte_count_ += key_prop_.length() + 3;

                            // continue the loop because the next character in the value may be another interpolated
                            // value
                            continue;
                        } else {
                            log(constants::PARSER_INTERPOLATION_ERROR);
                            std::exit(1);
                        }
                    }

                    value_ += line_slice[val_byte_count_];

                    ++val_byte_count_;
                }

                if (key_.length()) {
                    env_map_[key_] = value_;
                    if (options_.debug) {
                        log(constants::PARSER_DEBUG);
                    }
                }

                byte_count_ += assignment_index_ + val_byte_count_ + 1;
            } else {
                byte_count_ = file_.length();
            }
        }

        if (options_.debug) {
            log(constants::PARSER_DEBUG_FILE_PROCESSED);
        }

        env_file_.close();

        return this;
    }

    parser *parser::check_envs() {
        if (options_.required_envs.size()) {
            for (const std::string &key : options_.required_envs) {
                if (!env_map_.count(key) || !env_map_.at(key).length()) {
                    undefined_keys_.push_back(key);
                }
            }

            if (undefined_keys_.size()) {
                log(constants::PARSER_REQUIRED_ENV_ERROR);
                std::exit(1);
            }
        }

        if (!env_map_.size()) {
            log(constants::PARSER_EMPTY_ENVS);
            std::exit(1);
        }

        return this;
    }

    void parser::set_envs() {
        if (options_.commands.size()) {
            pid_t pid = fork();

            if (pid == 0) {
                for (const auto &[key, value] : env_map_) {
                    setenv(key.c_str(), value.c_str(), 0);
                }

                // NOTE: Unfortunately, have to set ENVs to the parent process because
                // "execvpe" doesn't exist on POSIX, but also it doesn't allow binaries
                // that were called to locate other binaries found within shell "PATH"
                // for example: "npm run dev" won't work because "npm" can't find "node"
                execvp(options_.commands[0], options_.commands.data());
                if (errno == ENOENT) {
                    std::cerr
                        << nvi::fmt::format(
                               "[nvi] (main::COMMAND_ENOENT_ERROR) The specified command encountered an error. The "
                               "command "
                               "\"%s\" doesn't appear to exist or may not reside in a directory within the shell PATH.",
                               options_.commands[0])
                        << std::endl;
                    _exit(-1);
                }
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                std::cerr << "[nvi] (main::COMMAND_FAILED_TO_START) Unable to run the specified command." << std::endl;
                std::exit(1);
            }
        } else {
            const std::string last_key = std::prev(env_map_.end())->first;
            std::cout << "{" << std::endl;
            for (auto const &[key, value] : env_map_) {
                std::string esc_value;
                size_t i = 0;
                while (i < value.size()) {
                    char current_char = value[i];
                    if (current_char == constants::DOUBLE_QUOTE) {
                        esc_value += constants::BACK_SLASH;
                    }
                    esc_value += current_char;
                    ++i;
                }
                const std::string comma = key != last_key ? "," : "";
                std::cout << std::setw(4) << "\"" << key << "\""
                          << ": \"" << esc_value << "\"" << comma << std::endl;
            }
            std::cout << "}" << std::endl;
        }
    }

    parser *parser::read(const std::string &env_file_name) {
        file_name_ = env_file_name;
        byte_count_ = 0;
        val_byte_count_ = 0;
        line_count_ = 0;
        assignment_index_ = -1;
        key_.clear();
        key_prop_.clear();
        value_.clear();
        file_.clear();
        file_path_.clear();

        file_path_ = std::string(std::filesystem::current_path() / options_.dir / file_name_);
        if (!std::filesystem::exists(file_path_)) {
            log(constants::PARSER_FILE_ERROR);
            std::exit(1);
        }

        env_file_ = std::ifstream(file_path_, std::ios_base::in);
        if (env_file_.bad()) {
            log(constants::PARSER_FILE_ERROR);
            std::exit(1);
        }
        file_ = std::string{std::istreambuf_iterator<char>(env_file_), std::istreambuf_iterator<char>()};
        if (!file_.length()) {
            log(constants::PARSER_EMPTY_ENVS);
            std::exit(1);
        }
        file_view_ = std::string_view(file_);

        return this;
    }

    parser *parser::parse_envs() noexcept {
        for (const std::string &env : options_.files) {
            read(env)->parse();
        }

        return this;
    }

    void parser::log(uint8_t code) const noexcept {
        switch (code) {
        case constants::PARSER_INTERPOLATION_WARNING: {
            std::clog << fmt::format(
                             "[nvi] (parser::INTERPOLATION_WARNING::%s:%d:%d) The key \"%s\" contains an invalid "
                             "interpolated variable: \"%s\". Unable to locate a value that corresponds to this key.",
                             file_name_.c_str(), line_count_ + 1, assignment_index_ + val_byte_count_ + 2, key_.c_str(),
                             key_prop_.c_str())
                      << std::endl;
            break;
        }
        case constants::PARSER_INTERPOLATION_ERROR: {
            std::cerr << fmt::format("[nvi] (parser::INTERPOLATION_ERROR::%s:%d:%d) The key \"%s\" contains an "
                                     "interpolated \"{\" operator, but appears to be missing a closing \"}\" operator.",
                                     file_name_.c_str(), line_count_ + 1, assignment_index_ + val_byte_count_ + 2,
                                     key_.c_str())
                      << std::endl;
            break;
        }
        case constants::PARSER_DEBUG: {
            std::clog << fmt::format("[nvi] (parser::DEBUG::%s:%d:%d) Set key \"%s\" to equal value \"%s\".",
                                     file_name_.c_str(), line_count_ + 1, assignment_index_ + val_byte_count_ + 2,
                                     key_.c_str(), value_.c_str())
                      << std::endl;
            break;
        }
        case constants::PARSER_DEBUG_FILE_PROCESSED: {
            std::clog << fmt::format("[nvi] (parser::DEBUG_FILE_PROCESSED::%s) Processed %d line%s and %d bytes!\n",
                                     file_name_.c_str(), line_count_, (line_count_ != 1 ? "s" : ""), byte_count_)
                      << std::endl;
            break;
        }
        case constants::PARSER_REQUIRED_ENV_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (parser::REQUIRED_ENV_ERROR) The following ENV keys are marked as required: \"%s\""
                             ", but they are undefined after all of the .env files were parsed.",
                             fmt::join(undefined_keys_, ", ").c_str())
                      << std::endl;
            break;
        }
        case constants::PARSER_FILE_ERROR: {
            std::cerr << fmt::format("[nvi] (parser::FILE_ERROR) Unable to locate \"%s\". The .env file doesn't appear "
                                     "to exist or isn't a valid file format!",
                                     file_path_.c_str())
                      << std::endl;
            break;
        }
        case constants::PARSER_EMPTY_ENVS: {
            std::cerr
                << fmt::format(
                       "[nvi] (parser::PARSER_EMPTY_ENVS) Unable to parse any ENVS! Please ensure the \"%s\" file "
                       "is not empty.",
                       file_name_.c_str())
                << std::endl;
            break;
        }
        default:
            break;
        }
    }
}; // namespace nvi
