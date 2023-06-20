#include "json.cpp"
#include "load_config.h"
#include <cassert>
#include <string>
#include <vector>

using std::string;
using std::vector;

int main() {
    nvi::config config("bin_test_only", "../../");

    const bool debug = config.get_debug();
    assert(debug == true);

    const string dir = config.get_dir();
    assert(dir == "custom/directory");

    const vector<string> env_files = config.get_files();
    const vector<string> files = {"test1.env", "test2.env", "test3.env"};
    assert(env_files == files);

    const vector<string> required_envs = config.get_required_envs();
    const vector<string> envs = {"TEST1", "TEST2", "TEST3"};
    assert(required_envs == envs);

    return 0;
}
