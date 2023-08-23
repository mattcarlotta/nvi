#include "lexer.h"
#include "parser.h"
#include "gtest/gtest.h"

class Lex_Env_File : public testing::Test {
    protected:
        static nvi::tokens_t tokens;

    public:
        static void SetUpTestSuite() {
            nvi::Lexer lexer({/* commands */ {},
                              /* config */ "",
                              /* debug */ false,
                              /* dir */ "../envs",
                              /* files */ {"simple.env"},
                              /* required_envs */ {}});
            tokens = lexer.parse_files()->get_tokens();
        }
};

nvi::tokens_t Lex_Env_File::tokens;

TEST_F(Lex_Env_File, consistent_lex_size) { EXPECT_EQ(tokens.size(), 11); }

TEST_F(Lex_Env_File, basic_env) {
    const auto token = tokens.at(0);
    EXPECT_EQ(token.key, "BASIC_ENV");
    EXPECT_EQ(token.file, "simple.env");
    EXPECT_EQ(token.values.size(), 1);
    const auto vt = token.values.at(0);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "true");
    EXPECT_EQ(vt.line, 1);
    EXPECT_EQ(vt.byte, 15);
}

TEST_F(Lex_Env_File, no_value) {
    const auto token = tokens.at(1);
    EXPECT_EQ(token.key, "NO_VALUE");
    EXPECT_EQ(token.file, "simple.env");
    EXPECT_EQ(token.values.size(), 1);
    const auto vt = token.values.at(0);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "");
    EXPECT_EQ(vt.line, 2);
    EXPECT_EQ(vt.byte, 10);
}

TEST_F(Lex_Env_File, multiline_value) {
    const auto token = tokens.at(3);
    EXPECT_EQ(token.key, "MULTI_LINE_KEY");
    EXPECT_EQ(token.file, "simple.env");
    EXPECT_EQ(token.values.size(), 5);
    auto vt = token.values.at(0);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "ssh-rsa BBBB");
    EXPECT_EQ(vt.line, 6);
    EXPECT_EQ(vt.byte, 28);
    vt = token.values.at(1);
    EXPECT_EQ(vt.type, nvi::ValueType::multiline);
    EXPECT_EQ(vt.value.value(), "Pl1P1");
    EXPECT_EQ(vt.line, 7);
    EXPECT_EQ(vt.byte, 6);
    vt = token.values.at(2);
    EXPECT_EQ(vt.type, nvi::ValueType::multiline);
    EXPECT_EQ(vt.value.value(), "A");
    EXPECT_EQ(vt.line, 8);
    EXPECT_EQ(vt.byte, 2);
    vt = token.values.at(3);
    EXPECT_EQ(vt.type, nvi::ValueType::multiline);
    EXPECT_EQ(vt.value.value(), "D+jk/3");
    EXPECT_EQ(vt.line, 9);
    EXPECT_EQ(vt.byte, 7);
    vt = token.values.at(4);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "Lf3Dw== test@example.com");
    EXPECT_EQ(vt.line, 10);
    EXPECT_EQ(vt.byte, 25);
}

TEST_F(Lex_Env_File, interp_value) {
    const auto token = tokens.at(4);
    EXPECT_EQ(token.key, "INTERP_VALUE");
    EXPECT_EQ(token.file, "simple.env");
    EXPECT_EQ(token.values.size(), 2);
    auto vt = token.values.at(0);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MESSAGE");
    EXPECT_EQ(vt.line, 11);
    EXPECT_EQ(vt.byte, 23);
    vt = token.values.at(1);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "world");
    EXPECT_EQ(vt.line, 11);
    EXPECT_EQ(vt.byte, 29);
}

TEST_F(Lex_Env_File, multi_interp_value) {
    const auto token = tokens.at(10);
    EXPECT_EQ(token.key, "MONGOLAB_URI");
    EXPECT_EQ(token.file, "simple.env");
    EXPECT_EQ(token.values.size(), 10);
    auto vt = token.values.at(0);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "mongodb://");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 24);
    vt = token.values.at(1);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MONGOLAB_USER");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 39);
    vt = token.values.at(2);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), ":");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 41);
    vt = token.values.at(3);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MONGOLAB_PASSWORD");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 60);
    vt = token.values.at(4);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "@");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 62);
    vt = token.values.at(5);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MONGOLAB_DOMAIN");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 79);
    vt = token.values.at(6);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), ":");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 81);
    vt = token.values.at(7);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MONGOLAB_PORT");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 96);
    vt = token.values.at(8);
    EXPECT_EQ(vt.type, nvi::ValueType::normal);
    EXPECT_EQ(vt.value.value(), "/");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 98);
    vt = token.values.at(9);
    EXPECT_EQ(vt.type, nvi::ValueType::interpolated);
    EXPECT_EQ(vt.value.value(), "MONGOLAB_DATABASE");
    EXPECT_EQ(vt.line, 18);
    EXPECT_EQ(vt.byte, 117);
}
