#include "lexer.h"
#include "options.h"
#include "parser.h"
#include "gtest/gtest.h"
#include <sstream>

class Lex_Env_File : public testing::Test {
    protected:
    static nvi::tokens_t file_tokens;

    public:
    static void SetUpTestSuite() {
        nvi::options_t options{/* api */ false,
                               /* commands */ {},
                               /* config */ "",
                               /* debug */ false,
                               /* dir */ "../envs",
                               /* environment */ "",
                               /* files */ {"simple.env"},
                               /* print */ false,
                               /* project */ "",
                               /* required_envs */ {},
                               /* save */ false};
        nvi::Lexer lexer(&options);
        file_tokens = lexer.parse_files()->get_tokens();
    }
};

nvi::tokens_t Lex_Env_File::file_tokens;

TEST_F(Lex_Env_File, consistent_file_lex_size) { EXPECT_EQ(file_tokens.size(), 11); }

TEST_F(Lex_Env_File, basic_env) {
    const auto token = file_tokens.at(0);
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
    const auto token = file_tokens.at(1);
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
    const auto token = file_tokens.at(3);
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
    const auto token = file_tokens.at(4);
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
    const auto token = file_tokens.at(10);
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

TEST(Lex_Response_Env, consistent_response_lex_size) {
    std::stringstream envs;
    envs << "API_KEY_1=sdfksdfj" << '\n';
    envs << "API_KEY_2=${API_KEY_1}" << '\n';
    envs << "API_KEY_3=ssh-rsa BBBB\\" << '\n';
    envs << "Pl1P1\\" << '\n';
    envs << "A\\" << '\n';
    envs << "D+jk/3\\" << '\n';
    envs << "Lf3Dw== test@example.com" << '\n';
    envs << "API_KEY_4='  SINGLE QUOTES  '" << '\n';
    envs << "API_KEY_5=\"  DOUBLE QUOTES  \"" << '\n';
    nvi::options_t options{/* api */ false,
                           /* commands */ {},
                           /* config */ "",
                           /* debug */ false,
                           /* dir */ "",
                           /* environment */ "",
                           /* files */ {},
                           /* print */ false,
                           /* project */ "",
                           /* required_envs */ {},
                           /* save */ false};

    nvi::Lexer lexer(&options);
    nvi::tokens_t res_tokens = lexer.parse_api_response(envs.str())->get_tokens();

    EXPECT_EQ(res_tokens.size(), 5);
}
