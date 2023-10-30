#include "arg.h"
#include "options.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

char *argv[] = {(char *)"tests",      (char *)"--api",         (char *)"--debug",   (char *)"--directory",
                (char *)"tests",      (char *)"--environment", (char *)"my_env",    (char *)"--files",
                (char *)"test.env",   (char *)"test2.env",     (char *)"--project", (char *)"my_project",
                (char *)"--required", (char *)"KEY1",          (char *)"KEY2",      (char *)"--save",
                (char *)"--",         (char *)"bin",           (char *)"test",      (char *)NULL};
int argc = 20;

nvi::options_t opts;
nvi::Arg args(argc, argv, opts);

TEST(Arg, parseable_api) { EXPECT_EQ(opts.api, true); }

TEST(Arg, parseable_config) { EXPECT_EQ(opts.config, ""); }

TEST(Arg, parseable_debug) { EXPECT_EQ(opts.debug, true); }

TEST(Arg, parseable_directory) { EXPECT_EQ(opts.dir, "tests"); }

TEST(Arg, parseable_env) { EXPECT_EQ(opts.environment, "my_env"); }

TEST(Arg, parseable_execute) { EXPECT_EQ(opts.commands.size(), 3); }

TEST(Arg, parseable_files) {
    const std::vector<std::string> files = {"test.env", "test2.env"};
    EXPECT_EQ(opts.files, files);
}

TEST(Arg, parseable_print) { EXPECT_EQ(opts.print, false); }

TEST(Arg, parseable_project) { EXPECT_EQ(opts.project, "my_project"); }

TEST(Arg, parseable_required_envs) {
    const std::vector<std::string> required_envs = {"KEY1", "KEY2"};
    EXPECT_EQ(opts.required_envs, required_envs);
}

TEST(Arg, parseable_save) { EXPECT_EQ(opts.save, true); }
