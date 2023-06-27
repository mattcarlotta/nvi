#include "json.cpp"
#include "load_config.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

nvi::config config("bin_test_only", "../../");

TEST(parse_config_file, debug) {
    const bool debug = config.get_debug();
    EXPECT_EQ(debug, true);
}

TEST(parse_config_file, directory) {
    const string dir = config.get_dir();
    EXPECT_EQ(dir, "custom/directory");
}

TEST(parse_config_file, files) {
    const vector<string> env_files = config.get_files();
    const vector<string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(env_files, files);

    const vector<string> required_envs = config.get_required_envs();
    const vector<string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(required_envs, envs);
}

TEST(parse_config_file, envs) {
    const vector<string> required_envs = config.get_required_envs();
    const vector<string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(required_envs, envs);
}
