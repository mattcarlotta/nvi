#include "arg.h"
#include "gtest/gtest.h"

TEST(args, are_parseable) {
    int argc = 3;
    char *argv[] = {(char *)"", (char *)"--file", (char *)"test.env"};

    nvi::arg_parser args(argc, argv);
    const std::string file = args.get("--file").value_or("");

    EXPECT_EQ(file, "test.env");
}
