#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;

using std::ios_base;
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
    std::map<string, string> args;

    public:
    ArgParser(int &argc, char *argv[]) {
        for (int i = 1; i < argc; i += 2)
            this->args.insert(std::make_pair(argv[i], argv[i + 1]));
    }

    const string get(const string &flag) {
        string arg = this->args[flag];
        if (!arg.length()) {
            const string empty_str{""};
            return empty_str;
        }

        return arg;
    }
};

int main(int argc, char *argv[]) {
    ArgParser args(argc, argv);

    const string dir = args.get("-d");
    const string file_name = args.get("-f");
    if (!file_name.length()) {
        throw std::invalid_argument("You must assign an .env file to read!");
    }

    fs::path env_path = fs::current_path().parent_path();
    env_path = env_path / dir / file_name;

    std::ifstream env_file(env_path.string(), ios_base::in);

    if (!env_file.good()) {
        throw ios_base::failure("File does not exist!");
    }

    unsigned int byteCount = 0;
    unsigned int lineCount = 0;
    string file{streambuf_char(env_file), streambuf_char()};

    while (byteCount < file.length()) {
        char current_char = file[byteCount];

        std::cout << current_char;

        ++byteCount;
    }

    env_file.close();

    return 0;
}
