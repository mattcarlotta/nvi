#include "env.h"
#include "json.cpp"
#include "gtest/gtest.h"
#include <iostream>

class parse_env_file : public testing::Test {
    protected:
    static nlohmann::json env_map;

    public:
    static void SetUpTestSuite() {
        nvi::file env_file("../../tests/envs", "bin.env", false);
        env_map = env_file.parse(env_map);
    }
};

nlohmann::json parse_env_file::env_map;

TEST_F(parse_env_file, size) {
    const int env_size = env_map.size();
    EXPECT_EQ(env_size, 27);
}

TEST_F(parse_env_file, basic_env) {
    const std::string value = env_map.at("BASIC_ENV");
    EXPECT_EQ(value, "true");
}

TEST_F(parse_env_file, multi_line_values) {
    const std::string value = env_map.at("MULTI_LINE_KEY");
    EXPECT_EQ(value, "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");
}

TEST_F(parse_env_file, default_empty_value) {
    const std::string value = env_map.at("NO_VALUE");
    EXPECT_EQ(value, "");
}

TEST_F(parse_env_file, hashed_values) {
    const std::string value = env_map.at("HASH_KEYS");
    EXPECT_EQ(value, "1#2#3#4#");
}

TEST_F(parse_env_file, dollar_sign_values) {
    const std::string value = env_map.at("JUST_DOLLARS");
    EXPECT_EQ(value, "$$$$$");
}

TEST_F(parse_env_file, braces_values) {
    const std::string value = env_map.at("JUST_BRACES");
    EXPECT_EQ(value, "{{{{}}}}");
}

TEST_F(parse_env_file, spaces_values) {
    const std::string value = env_map.at("JUST_SPACES");
    EXPECT_EQ(value, "          ");
}

TEST_F(parse_env_file, sentence_values) {
    const std::string value = env_map.at("SENTENCE");
    EXPECT_EQ(value, "chat gippity is just a junior engineer that copies/pastes from StackOverflow");
}

TEST_F(parse_env_file, empty_interp_key_values) {
    const std::string value = env_map.at("EMPTY_INTRP_KEY");
    EXPECT_EQ(value, "abc123");
}

TEST_F(parse_env_file, single_quote_values) {
    const std::string value = env_map.at("SINGLE_QUOTES_SPACES");
    EXPECT_EQ(value, "'  SINGLE QUOTES  '");
}

TEST_F(parse_env_file, double_quote_values) {
    const std::string value = env_map.at("DOUBLE_QUOTES_SPACES");
    EXPECT_EQ(value, "\"  DOUBLE QUOTES  \"");
}

TEST_F(parse_env_file, quote_values) {
    const std::string value = env_map.at("QUOTES");
    EXPECT_EQ(value, "sad\"wow\"bak");
}

TEST_F(parse_env_file, invalid_single_quote_values) {
    const std::string value = env_map.at("INVALID_SINGLE_QUOTES");
    EXPECT_EQ(value, "bad\'dq");
}

TEST_F(parse_env_file, invalid_double_quote_values) {
    const std::string value = env_map.at("INVALID_DOUBLE_QUOTES");
    EXPECT_EQ(value, "bad\"dq");
}

TEST_F(parse_env_file, invalid_template_literal_values) {
    const std::string value = env_map.at("INVALID_TEMPLATE_LITERAL");
    EXPECT_EQ(value, "bad`as");
}

TEST_F(parse_env_file, invalid_mix_quotes_values) {
    const std::string value = env_map.at("INVALID_MIX_QUOTES");
    EXPECT_EQ(value, "bad\"'`mq");
}

TEST_F(parse_env_file, empty_single_quotes_values) {
    const std::string value = env_map.at("EMPTY_SINGLE_QUOTES");
    EXPECT_EQ(value, "''");
}

TEST_F(parse_env_file, empty_double_quotes_values) {
    const std::string value = env_map.at("EMPTY_DOUBLE_QUOTES");
    EXPECT_EQ(value, "\"\"");
}

TEST_F(parse_env_file, empty_template_literal_values) {
    const std::string value = env_map.at("EMPTY_TEMPLATE_LITERAL");
    EXPECT_EQ(value, "``");
}

TEST_F(parse_env_file, interpolated_values) {
    const std::string value = env_map.at("MONGOLAB_URI");
    EXPECT_EQ(value, "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");
}
