#include "config.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

nvi::Config config("staggered_with_comments", "../envs");

TEST(Config, parseable_debug) { EXPECT_EQ(config.get_options().debug, true); }

TEST(Config, parseable_directory) { EXPECT_EQ(config.get_options().dir, "custom/directory"); }

TEST(Config, parseable_env) { EXPECT_EQ(config.get_options().environment, "my_env"); }

TEST(Config, parseable_execute) { EXPECT_EQ(config.get_options().commands.size(), 3); }

TEST(Config, parseable_files) {
    const std::vector<std::string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(config.get_options().files, files);
}

TEST(Config, parseable_project) { EXPECT_EQ(config.get_options().project, "my_project"); }

TEST(Config, parseable_required_envs) {
    const std::vector<std::string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(config.get_options().required_envs, envs);
}
