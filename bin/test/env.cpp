#include "../src/env.h"
#include "../src/json/single_include/nlohmann/json.hpp"
#include <cassert>

int main() {
    nlohmann::json env_map;

    nvi::file env_file("../../tests/envs", ".env");
    env_map = env_file.parse(env_map);

    const auto env_size = env_map.size();
    assert(env_size == 35);

    // parses basic env values
    const auto basic_value = env_map.at("BASIC_ENV");
    assert(basic_value == "true");

    // parses multi line values
    const auto multi_line_value = env_map.at("MULTI_LINE_KEY");
    assert(multi_line_value == "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com");

    // defaults empty values to empty string
    const auto no_value = env_map.at("NO_VALUE");
    assert(no_value == "");

    // parsed values with hashes
    const auto hash_keys = env_map.at("HASH_KEYS");
    assert(hash_keys == "1#2#3#4#");

    // parses values with just hashes
    const auto just_hashes = env_map.at("JUST_HASHES");
    assert(just_hashes == "#####");

    // parses values with just dollar signs
    const auto just_dollars = env_map.at("JUST_DOLLARS");
    assert(just_dollars == "$$$$$");

    // parses values with just braces
    const auto just_braces = env_map.at("JUST_BRACES");
    assert(just_braces == "{{{{}}}}");

    // parses values with just spaces
    const auto just_spaces = env_map.at("JUST_SPACES");
    assert(just_spaces == "          ");

    // parses sentence values
    const auto sentence = env_map.at("SENTENCE");
    assert(sentence == "chat gippity is just a junior engineer that copies/pastes from StackOverflow");

    // parses interpolated values from previous keys
    const auto interp = env_map.at("INTERP");
    assert(interp == "aatruecdhelloef");

    // retains values despite an invalid interpolated value
    const auto empty_interp_key = env_map.at("EMPTY_INTRP_KEY");
    assert(empty_interp_key == "abc123");

    // retains valid interpolated values but ignores invalid interpolated value
    const auto retain_valid_interp_key = env_map.at("CONTAINS_INVALID_INTRP_KEY");
    assert(retain_valid_interp_key == "hello");

    // defaults invalid interpolated values to an empty string
    const auto no_interp = env_map.at("NO_INTERP");
    assert(no_interp == "");

    // discards invalid interpolated values with non-delineated braces
    const auto bad_interp = env_map.at("BAD_INTERP");
    assert(bad_interp == "lmnotuv");

    // respects single quotes and spaces
    const auto single_quotes = env_map.at("SINGLE_QUOTES_SPACES");
    assert(single_quotes == "'  SINGLE QUOTES  '");

    // respects double quotes and spaces
    const auto double_quotes = env_map.at("DOUBLE_QUOTES_SPACES");
    assert(double_quotes == "\"  DOUBLE QUOTES  \"");

    // respects quotes within letters
    const auto quotes = env_map.at("QUOTES");
    assert(quotes == "sad\"wow\"bak");

    // handles invalid single quotes within letters
    const auto invalid_singl_quotes = env_map.at("INVALID_SINGLE_QUOTES");
    assert(invalid_singl_quotes == "bad\'dq");

    // handles invalid double quotes within letters
    const auto invalid_double_quotes = env_map.at("INVALID_DOUBLE_QUOTES");
    assert(invalid_double_quotes == "bad\"dq");

    // handles invalid template literals within letters
    const auto invalid_template_literal = env_map.at("INVALID_TEMPLATE_LITERAL");
    assert(invalid_template_literal == "bad`as");

    // handles invalid mix quotes within letters
    const auto invalid_mix_quotes = env_map.at("INVALID_MIX_QUOTES");
    assert(invalid_mix_quotes == "bad\"'`mq");

    // handles empty single quotes
    const auto empty_single_quotes = env_map.at("EMPTY_SINGLE_QUOTES");
    assert(empty_single_quotes == "''");

    // handles empty double quotes
    const auto empty_double_quotes = env_map.at("EMPTY_DOUBLE_QUOTES");
    assert(empty_double_quotes == "\"\"");

    // handles empty template literals
    const auto empty_template_literal = env_map.at("EMPTY_TEMPLATE_LITERAL");
    assert(empty_template_literal == "``");

    // concatenates values from previous keys in file
    const auto mongolab_URI = env_map.at("MONGOLAB_URI");
    assert(mongolab_URI == "mongodb://root:password@abcd1234.mongolab.com:12345/localhost");

    return 0;
}
