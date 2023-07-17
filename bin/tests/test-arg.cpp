#include "arg.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

char *argv[] = {(char *)"",           (char *)"--config", (char *)"test",     (char *)"--debug",
                (char *)"--exec",     (char *)"bin",      (char *)"test",     (char *)"--dir",
                (char *)"tests",      (char *)"--files",  (char *)"test.env", (char *)"test2.env",
                (char *)"--required", (char *)"KEY1",     (char *)"KEY2",     NULL};
int argc = 16;

nvi::arg_parser arg(argc, argv);

TEST(arg_parser, parseable_config) { EXPECT_EQ(arg.config, "test"); }

TEST(arg_parser, parseable_debug) { EXPECT_EQ(arg.debug, true); }

TEST(arg_parser, parseable_directory) { EXPECT_EQ(arg.dir, "tests"); }

TEST(arg_parser, parseable_execute) { EXPECT_EQ(arg.commands.size(), 3); }

TEST(arg_parser, parseable_files) {
    const std::vector<std::string> files = {"test.env", "test2.env"};
    EXPECT_EQ(arg.files, files);
}

TEST(arg_parser, parseable_required_envs) {
    const std::vector<std::string> required_envs = {"KEY1", "KEY2"};
    EXPECT_EQ(arg.required_envs, required_envs);
}
