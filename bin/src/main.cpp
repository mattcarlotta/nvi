#include "arg.h"
#include "config.h"
#include "json.cpp"
#include "parser.h"
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using std::string;
using std::vector;

std::string findBinaryPath(const std::string &binaryName) {
    std::string command = "which " + binaryName;

    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to execute the 'which' command." << std::endl;
        return "";
    }

    char buffer[128];
    std::string result = "";

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    // Remove trailing newline character if present
    if (!result.empty() && result[result.length() - 1] == '\n') {
        result.erase(result.length() - 1);
    }

    return result;
}

char **convertToCharArray(const std::map<std::string, std::string> &map) {
    char **charArray = new char *[map.size() + 1]; // +1 for the null terminator

    size_t index = 0;
    for (const auto &[key, value] : map) {
        const std::string entry = key + "=" + value;
        charArray[index] = new char[entry.size() + 1]; // +1 for the null terminator
        std::strcpy(charArray[index], entry.c_str());

        ++index;
    }

    charArray[map.size()] = nullptr; // Add null terminator

    return charArray;
}

void freeCharArray(char **charArray, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        delete[] charArray[i];
    }
    delete[] charArray;
}

int main(int argc, char *argv[]) {
    nvi::arg_parser arg(argc, argv);
    vector<string> files = arg.files;
    std::optional<string> dir = arg.dir;
    bool debug = arg.debug;
    vector<string> required_envs = arg.required_envs;

    if (arg.config.length()) {
        nvi::config config(&arg.config);
        dir = config.dir;
        debug = config.debug;
        files = config.files;
        required_envs = config.required_envs;
    }

    nvi::parser parser(&files, dir, &required_envs, debug);
    std::string binaryPath = findBinaryPath("npm");

    auto envs = parser.parse_envs()->env_map;
    char **charArray = convertToCharArray(envs);
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        execle(binaryPath.c_str(), "npm", "run", "dev", NULL, charArray);
        freeCharArray(charArray, envs.size() + 1);
    } else if (pid > 0) {
        // Parent process
        int status;
        wait(&status);
    } else {
        // Error occurred
        // Handle the error here
    }

    return 0;
}
