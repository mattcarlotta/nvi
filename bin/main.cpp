#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace fs = std::filesystem;

using std::ios_base;
using streambuf_char_t = std::istreambuf_iterator<char>;

const char COMMENT = '#';
const char LINE_DELIMITER = '\n';
const char BACK_SLASH = '\\';
const char ASSIGN_OP = '=';
const char DOLLAR_SIGN = '$';
const char OPEN_BRACE = '{';
const char CLOSE_BRACE = '}';

int main(int argc, char *argv[]) {
    if (argc < 3) {
        throw std::invalid_argument("You must include a directory to search and an .env file to read!");
    }

    const char *dir = argv[1];
    const char *file_name = argv[2];

    // TODO: Refactor this to simplify building a path string
    const fs::path env_path = fs::current_path().parent_path().concat("/").concat(dir).concat("/").concat(file_name);

    std::ifstream env_file(env_path.string(), ios_base::in);

    if (!env_file.good()) {
        throw ios_base::failure("File does not exist!");
    }

    unsigned int byteCount = 0;
    unsigned int lineCount = 0;
    std::string file{streambuf_char_t(env_file), streambuf_char_t()};

    while (byteCount < file.length()) {
        char current_char = file[byteCount];

        std::cout << current_char;

        ++byteCount;
    }

    env_file.close();

    return 0;
}
