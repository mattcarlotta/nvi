#include "config.h"
#include "json.cpp"
#include "gtest/gtest.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

const string env = "bin_test_only";
nvi::config config(&env, "../../");

TEST(parse_config_file, debug) { EXPECT_EQ(config.debug, true); }

TEST(parse_config_file, directory) { EXPECT_EQ(config.dir, "custom/directory"); }

TEST(parse_config_file, execute) { EXPECT_EQ(config.commands.size(), 3); }

TEST(parse_config_file, files) {
    const vector<string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(config.files, files);
}

TEST(parse_config_file, envs) {
    const vector<string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(config.required_envs, envs);
}
