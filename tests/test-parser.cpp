#include "lexer.h"
#include "options.h"
#include "parser.h"
#include "gtest/gtest.h"

class Parse_Env_File : public testing::Test {
    protected:
        static nvi::env_map_t env_map;

    public:
        static void SetUpTestSuite() {
            nvi::options_t options = {/* commands */ {},
                                      /* config */ "",
                                      /* debug */ false,
                                      /* dir */ "../envs",
                                      /* files */ {".env", "base.env", "reference.env"},
                                      /* required_envs */ {"BASIC_ENV"}};
            nvi::Lexer lexer(options);
            lexer.read_files();
            nvi::Parser parser(lexer.get_tokens(), options);
            env_map = parser.parse_tokens()->check_envs()->get_env_map();
        }
};

nvi::env_map_t Parse_Env_File::env_map;

TEST_F(Parse_Env_File, consistent_parse_size) { EXPECT_EQ(env_map.size(), 37); }

TEST_F(Parse_Env_File, basic_env) { EXPECT_EQ(env_map.at("BASIC_ENV"), "true"); }

TEST_F(Parse_Env_File, base_env) { EXPECT_EQ(env_map.at("BASE"), "hello"); }

TEST_F(Parse_Env_File, reference_base_env) { EXPECT_EQ(env_map.at("REFERENCE"), "hello"); }

TEST_F(Parse_Env_File, multi_line_values) {
    EXPECT_EQ(env_map.at("MULTI_LINE_KEY"), "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");
}

TEST_F(Parse_Env_File, default_empty_value) { EXPECT_EQ(env_map.at("NO_VALUE"), ""); }

TEST_F(Parse_Env_File, hashed_values) { EXPECT_EQ(env_map.at("HASH_KEYS"), "1#2#3#4#"); }

TEST_F(Parse_Env_File, dollar_sign_values) { EXPECT_EQ(env_map.at("JUST_DOLLARS"), "$$$$$"); }

TEST_F(Parse_Env_File, braces_values) { EXPECT_EQ(env_map.at("JUST_BRACES"), "{{{{}}}}"); }

TEST_F(Parse_Env_File, spaces_values) { EXPECT_EQ(env_map.at("JUST_SPACES"), "          "); }

TEST_F(Parse_Env_File, sentence_values) {
    EXPECT_EQ(env_map.at("SENTENCE"), "chat gippity is just a junior engineer that copies/pastes from StackOverflow");
}

TEST_F(Parse_Env_File, interp_env_from_process) { EXPECT_EQ(env_map.at("INTERP_ENV_FROM_PROCESS").length() > 0, true); }

TEST_F(Parse_Env_File, empty_interp_key_values) { EXPECT_EQ(env_map.at("EMPTY_INTRP_KEY"), "abc123"); }

TEST_F(Parse_Env_File, single_quote_values) { EXPECT_EQ(env_map.at("SINGLE_QUOTES_SPACES"), "'  SINGLE QUOTES  '"); }

TEST_F(Parse_Env_File, double_quote_values) { EXPECT_EQ(env_map.at("DOUBLE_QUOTES_SPACES"), "\"  DOUBLE QUOTES  \""); }

TEST_F(Parse_Env_File, quote_values) { EXPECT_EQ(env_map.at("QUOTES"), "sad\"wow\"bak"); }

TEST_F(Parse_Env_File, invalid_single_quote_values) { EXPECT_EQ(env_map.at("INVALID_SINGLE_QUOTES"), "bad\'dq"); }

TEST_F(Parse_Env_File, invalid_double_quote_values) { EXPECT_EQ(env_map.at("INVALID_DOUBLE_QUOTES"), "bad\"dq"); }

TEST_F(Parse_Env_File, invalid_template_literal_values) { EXPECT_EQ(env_map.at("INVALID_TEMPLATE_LITERAL"), "bad`as"); }

TEST_F(Parse_Env_File, invalid_mix_quotes_values) { EXPECT_EQ(env_map.at("INVALID_MIX_QUOTES"), "bad\"'`mq"); }

TEST_F(Parse_Env_File, empty_single_quotes_values) { EXPECT_EQ(env_map.at("EMPTY_SINGLE_QUOTES"), "''"); }

TEST_F(Parse_Env_File, empty_double_quotes_values) { EXPECT_EQ(env_map.at("EMPTY_DOUBLE_QUOTES"), "\"\""); }

TEST_F(Parse_Env_File, empty_template_literal_values) { EXPECT_EQ(env_map.at("EMPTY_TEMPLATE_LITERAL"), "``"); }

TEST_F(Parse_Env_File, interpolated_values) {
    EXPECT_EQ(env_map.at("MONGOLAB_URI"), "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");
}
