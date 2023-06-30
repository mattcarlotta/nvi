#include "arg.h"
#include "gtest/gtest.h"

char *argv[] = {(char *)"",          (char *)"--config",   (char *)"test",    (char *)"--debug",
                (char *)"--dir",     (char *)"tests",      (char *)"--files", (char *)"test.env",
                (char *)"test2.env", (char *)"--required", (char *)"KEY1",    (char *)"KEY2"};
int argc = 12;

nvi::arg_parser arg(argc, argv);

TEST(arg_parser, parseable_config) { EXPECT_EQ(arg.config, "test"); }

TEST(arg_parser, parseable_debug) { EXPECT_EQ(arg.debug, true); }

TEST(arg_parser, parseable_directory) { EXPECT_EQ(arg.dir, "tests"); }

TEST(arg_parser, parseable_files) {
    EXPECT_EQ(arg.files[0], "test.env");
    EXPECT_EQ(arg.files[1], "test2.env");
}

TEST(arg_parser, parseable_required_envs) {
    EXPECT_EQ(arg.required_envs[0], "KEY1");
    EXPECT_EQ(arg.required_envs[1], "KEY2");
}
