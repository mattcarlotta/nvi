#include "config.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

nvi::Config config("staggered_with_comments", "../envs");

TEST(parse_config_file, debug) { EXPECT_EQ(config.get_options().debug, true); }

TEST(parse_config_file, directory) { EXPECT_EQ(config.get_options().dir, "custom/directory"); }

TEST(parse_config_file, execute) { EXPECT_EQ(config.get_options().commands.size(), 3); }

TEST(parse_config_file, files) {
    const std::vector<std::string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(config.get_options().files, files);
}

TEST(parse_config_file, envs) {
    const std::vector<std::string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(config.get_options().required_envs, envs);
}
