#include "json/single_include/nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;

using json = nlohmann::json;
using std::ifstream;
using std::ios_base;
using std::map;
using std::string;
using streambuf_char = std::istreambuf_iterator<char>;

const char COMMENT = '#';
const char LINE_DELIMITER = '\n';
const char BACK_SLASH = '\\';
const char ASSIGN_OP = '=';
const char DOLLAR_SIGN = '$';
const char OPEN_BRACE = '{';
const char CLOSE_BRACE = '}';

class ArgParser {
    private:
    map<string, string> args;

    public:
    ArgParser(int &argc, char *argv[]) {
        for (int i = 1; i < argc; i += 2)
            this->args.insert(std::make_pair(argv[i], argv[i + 1]));
    }

    const string get(const string &flag) {
        try {
            return this->args.at(flag);
        } catch (const std::out_of_range &) {
            return "";
        }
    }
};

class EnvFileParser {
    private:
    ifstream env_file;
    string file;
    fs::path file_path;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;
    json env_map;

    public:
    EnvFileParser(fs::path &env_path, json &env_map) {
        this->file_path = env_path;
        this->env_file = ifstream(this->file_path.string(), ios_base::in);
        this->file = string{streambuf_char(this->env_file), streambuf_char()};
        this->env_map = env_map;
    }

    const bool exists() {
        if (this->env_file.good()) {
            return true;
        } else {
            std::cerr << "Unable to locate: '" << this->file_path.string() << "'. The file doesn't appear to exist!"
                      << std::endl;
            exit(1);
        }
    }

    json parse() {
        while (this->byte_count < this->file.length()) {
            const string line = file.substr(this->byte_count, this->file.length());
            const int line_delimiter_index = line.find(LINE_DELIMITER);
            if (line[0] == COMMENT || line[0] == LINE_DELIMITER) {
                ++this->line_count;
                this->byte_count += line_delimiter_index + 1;
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
                        ++this->line_count;
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
                            ++this->line_count;
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
                    this->env_map[key] = value;
                }

                this->byte_count += assignment_index + val_byte_count + 1;
            } else {
                this->byte_count = file.length();
            }
        }

        this->env_file.close();

        return env_map;
    }
};

int main(int argc, char *argv[]) {
    ArgParser args(argc, argv);

    const string file_name = args.get("-f");
    if (!file_name.length()) {
        std::cerr << "You must assign an .env file to read!" << std::endl;
        exit(1);
    }

    json env_map;

    fs::path env_path = fs::current_path() / args.get("-d") / file_name;

    EnvFileParser file(env_path, env_map);

    if (file.exists()) {
        env_map = file.parse();
    }

    std::cout << env_map << std::endl;

    return 0;
}
