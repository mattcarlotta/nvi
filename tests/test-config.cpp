#include "config.h"
#include "options.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

nvi::Config config("staggered_with_comments", "../envs");
nvi::options_t opts = config.generate_options()->get_options();
std::vector<nvi::ConfigToken> tokens = config.get_tokens();

TEST(Config, generate_tokens) { EXPECT_EQ(tokens.size(), 10); }

TEST(Config, parseable_api) {
    const nvi::ConfigToken token = tokens.at(0);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "api");
    EXPECT_EQ(opts.api, true);
}

TEST(Config, parseable_debug) {
    const nvi::ConfigToken token = tokens.at(1);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "debug");
    EXPECT_EQ(opts.debug, true);
}

TEST(Config, parseable_directory) {
    const nvi::ConfigToken token = tokens.at(2);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "directory");
    EXPECT_EQ(opts.dir, "path/to/custom/directory");
}

TEST(Config, parseable_env) {
    const nvi::ConfigToken token = tokens.at(3);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "environment");
    EXPECT_EQ(opts.environment, "my_env");
}

TEST(Config, parseable_execute) {
    const nvi::ConfigToken token = tokens.at(4);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "execute");
    EXPECT_EQ(opts.commands.size(), 3);
}

TEST(Config, parseable_files) {
    const nvi::ConfigToken token = tokens.at(5);
    EXPECT_EQ(token.type, nvi::ConfigValueType::array);
    EXPECT_EQ(token.key, "files");
    const std::vector<std::string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(opts.files, files);
}

TEST(Config, parseable_print) {
    const nvi::ConfigToken token = tokens.at(6);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "print");
    EXPECT_EQ(opts.print, true);
}

TEST(Config, parseable_project) {
    const nvi::ConfigToken token = tokens.at(7);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "project");
    EXPECT_EQ(opts.project, "my_project");
}

TEST(Config, parseable_required_envs) {
    const nvi::ConfigToken token = tokens.at(8);
    EXPECT_EQ(token.type, nvi::ConfigValueType::array);
    EXPECT_EQ(token.key, "required");

    const std::vector<std::string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(opts.required_envs, envs);
}

TEST(Config, parseable_save) {
    const nvi::ConfigToken token = tokens.at(9);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "save");
    EXPECT_EQ(opts.debug, true);
}
