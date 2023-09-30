#include "arg.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

char *argv[] = {(char *)"tests",      (char *)"--config",   (char *)"test",      (char *)"--debug",
                (char *)"--dir",      (char *)"tests",      (char *)"--env",     (char *)"my_env",
                (char *)"--files",    (char *)"test.env",   (char *)"test2.env", (char *)"--project",
                (char *)"my_project", (char *)"--required", (char *)"KEY1",      (char *)"KEY2",
                (char *)"--save",     (char *)"--",         (char *)"bin",       (char *)"test",
                (char *)NULL};
int argc = 21;

nvi::Arg args(argc, argv);

TEST(Arg, parseable_config) { EXPECT_EQ(args.get_options().config, "test"); }

TEST(Arg, parseable_debug) { EXPECT_EQ(args.get_options().debug, true); }

TEST(Arg, parseable_directory) { EXPECT_EQ(args.get_options().dir, "tests"); }

TEST(Arg, parseable_env) { EXPECT_EQ(args.get_options().environment, "my_env"); }

TEST(Arg, parseable_execute) { EXPECT_EQ(args.get_options().commands.size(), 3); }

TEST(Arg, parseable_files) {
    const std::vector<std::string> files = {"test.env", "test2.env"};
    EXPECT_EQ(args.get_options().files, files);
}

TEST(Arg, parseable_project) { EXPECT_EQ(args.get_options().project, "my_project"); }

TEST(Arg, parseable_required_envs) {
    const std::vector<std::string> required_envs = {"KEY1", "KEY2"};
    EXPECT_EQ(args.get_options().required_envs, required_envs);
}

TEST(Arg, parseable_save) { EXPECT_EQ(args.get_options().save, true); }
