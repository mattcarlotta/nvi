#include "config.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

nvi::Config config("staggered_with_comments", "../envs");

TEST(config_file, parseable_debug) { EXPECT_EQ(config.get_options().debug, true); }

TEST(config_file, parseable_directory) { EXPECT_EQ(config.get_options().dir, "custom/directory"); }

TEST(config_file, parseable_env) { EXPECT_EQ(config.get_options().environment, "my_env"); }

TEST(config_file, parseable_execute) { EXPECT_EQ(config.get_options().commands.size(), 3); }

TEST(config_file, parseable_files) {
    const std::vector<std::string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(config.get_options().files, files);
}

TEST(config_file, parseable_project) { EXPECT_EQ(config.get_options().project, "my_project"); }

TEST(config_file, parseable_required_envs) {
    const std::vector<std::string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(config.get_options().required_envs, envs);
}
