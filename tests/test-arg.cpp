#include "arg.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

char *argv[] = {(char *)"tests",      (char *)"--config", (char *)"test",     (char *)"--debug",
                (char *)"--exec",     (char *)"bin",      (char *)"test",     (char *)"--dir",
                (char *)"tests",      (char *)"--files",  (char *)"test.env", (char *)"test2.env",
                (char *)"--required", (char *)"KEY1",     (char *)"KEY2",     (char *)NULL};
int argc = 16;

nvi::Arg args(argc, argv);

TEST(arg, parseable_config) { EXPECT_EQ(args.get_options().config, "test"); }

TEST(arg, parseable_debug) { EXPECT_EQ(args.get_options().debug, true); }

TEST(arg, parseable_directory) { EXPECT_EQ(args.get_options().dir, "tests"); }

TEST(arg, parseable_execute) { EXPECT_EQ(args.get_options().commands.size(), 3); }

TEST(arg, parseable_files) {
    const std::vector<std::string> files = {"test.env", "test2.env"};
    EXPECT_EQ(args.get_options().files, files);
}

TEST(arg, parseable_required_envs) {
    const std::vector<std::string> required_envs = {"KEY1", "KEY2"};
    EXPECT_EQ(args.get_options().required_envs, required_envs);
}
