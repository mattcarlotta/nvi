#include "config.h"
#include "config_token.h"
#include "options.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

class Parse_Config_File : public testing::Test {
    protected:
    static nvi::options_t config_options;
    static nvi::config_tokens_t tokens;

    public:
    static void SetUpTestSuite() {
        config_options.config = "staggered_with_comments";
        config_options.dir = "../envs";
        nvi::Config config(config_options);
        config.generate_options();
        tokens = config.get_tokens();
    }
};

nvi::options_t Parse_Config_File::config_options;
nvi::config_tokens_t Parse_Config_File::tokens;

TEST_F(Parse_Config_File, generate_tokens) { EXPECT_EQ(tokens.size(), 10); }

TEST_F(Parse_Config_File, parseable_api) {
    const nvi::ConfigToken token = tokens.at(0);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "api");
    EXPECT_EQ(config_options.api, true);
}

TEST_F(Parse_Config_File, parseable_debug) {
    const nvi::ConfigToken token = tokens.at(1);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "debug");
    EXPECT_EQ(config_options.debug, true);
}

TEST_F(Parse_Config_File, parseable_directory) {
    const nvi::ConfigToken token = tokens.at(2);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "directory");
    EXPECT_EQ(config_options.dir, "path/to/custom/directory");
}

TEST_F(Parse_Config_File, parseable_env) {
    const nvi::ConfigToken token = tokens.at(3);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "environment");
    EXPECT_EQ(config_options.environment, "my_env");
}

TEST_F(Parse_Config_File, parseable_execute) {
    const nvi::ConfigToken token = tokens.at(4);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "execute");
    EXPECT_EQ(config_options.commands.size(), 3);
}

TEST_F(Parse_Config_File, parseable_files) {
    const nvi::ConfigToken token = tokens.at(5);
    EXPECT_EQ(token.type, nvi::ConfigValueType::array);
    EXPECT_EQ(token.key, "files");
    const std::vector<std::string> files = {"test1.env", "test2.env", "test3.env"};
    EXPECT_EQ(config_options.files, files);
}

TEST_F(Parse_Config_File, parseable_print) {
    const nvi::ConfigToken token = tokens.at(6);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "print");
    EXPECT_EQ(config_options.print, true);
}

TEST_F(Parse_Config_File, parseable_project) {
    const nvi::ConfigToken token = tokens.at(7);
    EXPECT_EQ(token.type, nvi::ConfigValueType::string);
    EXPECT_EQ(token.key, "project");
    EXPECT_EQ(config_options.project, "my_project");
}

TEST_F(Parse_Config_File, parseable_required_envs) {
    const nvi::ConfigToken token = tokens.at(8);
    EXPECT_EQ(token.type, nvi::ConfigValueType::array);
    EXPECT_EQ(token.key, "required");

    const std::vector<std::string> envs = {"TEST1", "TEST2", "TEST3"};
    EXPECT_EQ(config_options.required_envs, envs);
}

TEST_F(Parse_Config_File, parseable_save) {
    const nvi::ConfigToken token = tokens.at(9);
    EXPECT_EQ(token.type, nvi::ConfigValueType::boolean);
    EXPECT_EQ(token.key, "save");
    EXPECT_EQ(config_options.debug, true);
}
