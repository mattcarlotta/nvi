#include <cstdlib>
#include <iostream>
#include <string>

void assert(const std::string &key, const std::string &value) {
    const char *env_value = std::getenv(key.c_str());
    const std::string expected_value = (env_value != nullptr ? std::string(env_value) : "");

    if (expected_value == value) {
        return;
    }

    std::cerr << "ERROR: Expected the \"" << key << "\" value to match \"" << value << "\". Actual value: \""
              << expected_value << "\"." << std::endl;
    std::exit(EXIT_FAILURE);
}

int main() {
    assert("BASIC_ENV", "true");
    assert("QUOTES", "sad\"wow\"bak");
    assert("MULTI_LINE_KEY", "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");
    assert("MONGOLAB_URI", "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");

    std::clog << "Successfully assigned ENVs to the process!" << std::endl;
    return 0;
}
