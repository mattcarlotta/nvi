#include "json.cpp"
#include "parser.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>

class parse_env_file : public testing::Test {
    protected:
    static nlohmann::json env_map;

    public:
    static void SetUpTestSuite() {
        nvi::parser parser("../../tests/envs");
        parser.read("bin.env")->parse();
        env_map = parser.env_map;
    }
};

nlohmann::json parse_env_file::env_map;

TEST_F(parse_env_file, consistent_parse_size) { EXPECT_EQ(env_map.size(), 27); }

TEST_F(parse_env_file, basic_env) { EXPECT_EQ(env_map.at("BASIC_ENV"), "true"); }

TEST_F(parse_env_file, multi_line_values) {
    EXPECT_EQ(env_map.at("MULTI_LINE_KEY"), "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");
}

TEST_F(parse_env_file, default_empty_value) { EXPECT_EQ(env_map.at("NO_VALUE"), ""); }

TEST_F(parse_env_file, hashed_values) { EXPECT_EQ(env_map.at("HASH_KEYS"), "1#2#3#4#"); }

TEST_F(parse_env_file, dollar_sign_values) { EXPECT_EQ(env_map.at("JUST_DOLLARS"), "$$$$$"); }

TEST_F(parse_env_file, braces_values) { EXPECT_EQ(env_map.at("JUST_BRACES"), "{{{{}}}}"); }

TEST_F(parse_env_file, spaces_values) { EXPECT_EQ(env_map.at("JUST_SPACES"), "          "); }

TEST_F(parse_env_file, sentence_values) {
    EXPECT_EQ(env_map.at("SENTENCE"), "chat gippity is just a junior engineer that copies/pastes from StackOverflow");
}

TEST_F(parse_env_file, empty_interp_key_values) { EXPECT_EQ(env_map.at("EMPTY_INTRP_KEY"), "abc123"); }

TEST_F(parse_env_file, single_quote_values) { EXPECT_EQ(env_map.at("SINGLE_QUOTES_SPACES"), "'  SINGLE QUOTES  '"); }

TEST_F(parse_env_file, double_quote_values) { EXPECT_EQ(env_map.at("DOUBLE_QUOTES_SPACES"), "\"  DOUBLE QUOTES  \""); }

TEST_F(parse_env_file, quote_values) { EXPECT_EQ(env_map.at("QUOTES"), "sad\"wow\"bak"); }

TEST_F(parse_env_file, invalid_single_quote_values) { EXPECT_EQ(env_map.at("INVALID_SINGLE_QUOTES"), "bad\'dq"); }

TEST_F(parse_env_file, invalid_double_quote_values) { EXPECT_EQ(env_map.at("INVALID_DOUBLE_QUOTES"), "bad\"dq"); }

TEST_F(parse_env_file, invalid_template_literal_values) { EXPECT_EQ(env_map.at("INVALID_TEMPLATE_LITERAL"), "bad`as"); }

TEST_F(parse_env_file, invalid_mix_quotes_values) { EXPECT_EQ(env_map.at("INVALID_MIX_QUOTES"), "bad\"'`mq"); }

TEST_F(parse_env_file, empty_single_quotes_values) { EXPECT_EQ(env_map.at("EMPTY_SINGLE_QUOTES"), "''"); }

TEST_F(parse_env_file, empty_double_quotes_values) { EXPECT_EQ(env_map.at("EMPTY_DOUBLE_QUOTES"), "\"\""); }

TEST_F(parse_env_file, empty_template_literal_values) { EXPECT_EQ(env_map.at("EMPTY_TEMPLATE_LITERAL"), "``"); }

TEST_F(parse_env_file, interpolated_values) {
    EXPECT_EQ(env_map.at("MONGOLAB_URI"), "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");
}
