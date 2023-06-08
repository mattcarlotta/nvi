#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;

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

int main(int argc, char *argv[]) {
    ArgParser args(argc, argv);

    const string file_name = args.get("-f");
    if (!file_name.length()) {
        std::cerr << "You must assign an .env file to read!" << std::endl;
        return 1;
    }

    fs::path env_path = fs::current_path().parent_path() / args.get("-d") / file_name;

    std::ifstream env_file(env_path.string(), ios_base::in);

    if (!env_file.good()) {
        std::cerr << "Unable to locate: '" << env_path.string() << "'. The file doesn't appear to exist!" << std::endl;
        return 1;
    }

    string file{streambuf_char(env_file), streambuf_char()};

    map<string, string> env_map;
    unsigned int byte_count = 0;
    unsigned int line_count = 0;
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
                        try {
                            interpolated_value = env_map.at(key_prop);
                        } catch (const std::out_of_range &) {
                            interpolated_value = "";
                            std::cout << "The key '" << key << "' contains an invalid interpolated variable: '"
                                      << key_prop << "'. Unable to locate a value that corresponds to this key."
                                      << std::endl;
                        }

                        value += interpolated_value;

                        val_byte_count += key_prop.length() + 3;

                        continue;
                    } else {
                        ++line_count;
                        std::cerr << "The key '" << key
                                  << "' contains an interpolated variable: '${' operator but appears to be missing a "
                                     "closing '}' operator."
                                  << std::endl;
                        return 1;
                    }
                }

                value += line_slice[val_byte_count];

                ++val_byte_count;
            }

            if (key.length()) {
                env_map.insert(std::make_pair(key, value));
            }

            byte_count += assignment_index + val_byte_count + 1;
        } else {
            byte_count = file.length();
        }
    }

    std::cout << "Processed " << line_count << " line(s) and " << byte_count << " bytes!" << std::endl;

    for (auto const &[key, val] : env_map) {
        std::cout << key << ": " << val << std::endl;
    }

    env_file.close();

    return 0;
}
