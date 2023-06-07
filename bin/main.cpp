#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
// #include <string>

namespace fs = std::filesystem;

using std::ios_base;
using streambuf_char_t = std::istreambuf_iterator<char>;

const unsigned int DELIMITER = 0x0a;
const char NEW_LINE = '\n';

int main() {
    fs::path bin_dir = fs::current_path();
    // TODO: Change this from hard-coded to read from "env.config.json" or from args
    fs::path env_path = bin_dir.parent_path().concat("/.env");

    std::ifstream env_file(env_path.string(), ios_base::binary);

    if (!env_file.good()) {
        throw ios_base::failure("File does not exist!");
        // std::cout << "Unable to load " << env_path.string() << " because it does not exist!" << '\n';
        // return 1;
    } else {
        std::vector<char> file_buf{streambuf_char_t(env_file), streambuf_char_t()};

        for (int i = 0; i < file_buf.size(); ++i) {
            // if (file_buf[i] == NEW_LINE) {
            //     std::cout << "Found new line" << '\n';
            // }
            unsigned int byte = file_buf[i];
            // std::cout << std::setbase(16) << std::setfill('0') << std::setw(2) << byte << '\n';
            std::cout << std::setw(2) << std::setfill('0') << std::hex << byte << '\n';
        }
    }

    env_file.close();

    // auto endOfFile = envFile.tellg();
    // envFile.seekg(0, std::ios::beg);

    return 0;
}
