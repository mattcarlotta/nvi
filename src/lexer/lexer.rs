use super::{LexerToken, LexerValue, LexerValueToken};
use crate::globals;
use crate::logger::Logger;
use crate::options::OptionsType;
use std::fs;
use std::path::PathBuf;

pub struct Lexer<'a> {
    options: &'a OptionsType,
    index: usize,
    byte: usize,
    line: usize,
    file: String,
    file_name: String,
    file_path: PathBuf,
    tokens: Vec<LexerToken>,
    logger: Logger<'a>,
}

impl<'a> Lexer<'a> {
    pub fn new(options: &'a OptionsType) -> Self {
        let mut logger = Logger::new("Lexer");
        logger.set_debug(&options.debug);

        return Lexer {
            options,
            index: 0,
            byte: 1,
            line: 1,
            file: String::new(),
            file_name: String::new(),
            file_path: globals::current_dir().clone(),
            tokens: vec![],
            logger,
        };
    }

    pub fn get_tokens(self) -> Vec<LexerToken> {
        return self.tokens;
    }

    fn reset_byte(&mut self) {
        self.byte = 1;
    }

    fn inc_byte(&mut self, offset: Option<usize>) {
        self.byte += offset.unwrap_or(1);
    }

    fn inc_line(&mut self) {
        self.line += 1;
    }

    fn peek(&self, offset: Option<usize>) -> Option<char> {
        let index = self.index + offset.unwrap_or(0);
        if index >= self.file.len() {
            return None;
        }

        match self.file.chars().nth(index) {
            Some(c) => return Some(c),
            None => return None,
        }
    }

    fn skip(&mut self, offset: Option<usize>) {
        let o = offset.unwrap_or(1);
        self.inc_byte(Some(o));
        self.index += o;
    }

    fn get_char(&self, offset: Option<usize>) -> char {
        return match self.peek(offset) {
            Some(c) => c,
            None => globals::NULL_CHAR,
        };
    }

    fn parse(&mut self) {
        let mut token = LexerToken::new();
        token.file = self.file_name.to_owned();

        let mut value = String::new();
        while self.peek(None).is_some() {
            let current_char = self.get_char(None);

            match current_char {
                globals::NULL_CHAR => self.skip(None),
                globals::LINE_DELIMITER => {
                    if token.key.is_some() {
                        token.values.push(LexerValueToken::new(
                            LexerValue::Normal,
                            &value,
                            self.line,
                            self.byte,
                        ));
                        self.tokens.push(token.clone());
                    }

                    token.key = None;
                    token.values.clear();
                    value.clear();
                    self.inc_line();
                    self.skip(None);
                    self.reset_byte();
                }
                globals::ASSIGN_OP => {
                    token.key = Some(value.to_owned());
                    value.clear();

                    // skip '='
                    self.skip(None);
                }
                globals::HASH => {
                    // commit values with hashes
                    if token.key.is_some() {
                        value.push(current_char);
                        self.skip(None);
                        continue;
                    }

                    // skip lines starting with comments
                    while self.peek(None).is_some()
                        && self.get_char(None) != globals::LINE_DELIMITER
                    {
                        value.push(self.get_char(None));
                        self.skip(None);
                    }

                    token.values.push(LexerValueToken::new(
                        LexerValue::Comment,
                        &value,
                        self.line,
                        self.byte,
                    ));
                    self.tokens.push(token.clone());
                    value.clear();
                }
                globals::DOLLAR_SIGN => {
                    // commit values with dollar signs
                    if self.get_char(Some(1)) != globals::OPEN_BRACE {
                        value.push(current_char);
                        self.skip(None);
                        continue;
                    }

                    // store any previous parsed values
                    if !value.is_empty() {
                        token.values.push(LexerValueToken::new(
                            LexerValue::Normal,
                            &value,
                            self.line,
                            self.byte,
                        ));
                        value.clear();
                    }

                    // skip "${"
                    self.skip(Some(2));

                    // extract ENV within "${ENV}"
                    while self.peek(None).is_some() {
                        match self.get_char(None) {
                            globals::CLOSE_BRACE => {
                                // skip '}'
                                self.skip(None);
                                break;
                            }
                            globals::LINE_DELIMITER => {
                                self.logger.fatal(
                                    format!(
                                        "found an error in {}:{}:{}. The key {:?} contains an interpolated \"{{\" operator, but appears to be missing a closing \"}}\" operator.", 
                                        self.file_name, self.line, self.byte, token.key.unwrap_or(String::new())
                                    )
                                );
                            }
                            _ => {
                                if let Some(c) = self.peek(None) {
                                    value.push(c);
                                }
                                self.skip(None);
                            }
                        }
                    }

                    token.values.push(LexerValueToken::new(
                        LexerValue::Interpolated,
                        &value,
                        self.line,
                        self.byte,
                    ));

                    if self.get_char(None) == globals::LINE_DELIMITER {
                        self.tokens.push(token.clone());

                        token.key = None;
                        token.values.clear();
                        self.inc_line();
                        // skip '\n'
                        self.skip(None);
                        self.reset_byte();
                    }

                    value.clear();
                }
                globals::BACK_SLASH => {
                    // commit values with back slashes
                    if self.peek(Some(1)).is_some()
                        && self.get_char(Some(1)) != globals::LINE_DELIMITER
                    {
                        value.push(current_char);
                        continue;
                    }

                    // store any previous parsed values
                    if !value.is_empty() {
                        token.values.push(LexerValueToken::new(
                            LexerValue::Normal,
                            &value,
                            self.line,
                            self.byte,
                        ));
                    }

                    // skip "\\n" characters
                    self.skip(Some(2));
                    self.reset_byte();

                    value.clear();

                    // handle multiline values
                    while self.peek(None).is_some() {
                        let is_eol = self.get_char(None) == globals::LINE_DELIMITER;

                        if (self.get_char(None) == globals::BACK_SLASH
                            && self.get_char(Some(1)) == globals::LINE_DELIMITER)
                            || is_eol
                        {
                            self.inc_line();

                            token.values.push(LexerValueToken::new(
                                LexerValue::Multiline,
                                &value,
                                self.line,
                                self.byte,
                            ));

                            value.clear();
                            self.reset_byte();

                            // skip '\n' or "\\n"
                            let mut next_byte: usize = 1;
                            if !is_eol {
                                next_byte += 1;
                            }
                            self.skip(Some(next_byte));

                            if is_eol {
                                break;
                            }
                        }

                        if let Some(c) = self.peek(None) {
                            value.push(c);
                        }
                        self.skip(None);
                    }

                    self.tokens.push(token.clone());
                    token.key = None;
                    token.values.clear();
                    value.clear();
                    self.reset_byte();
                    self.inc_line();
                }
                _ => {
                    value.push(current_char);
                    self.skip(None);
                }
            }
        }

        if self.tokens.is_empty() {
            self.logger.fatal(format!(
                "was unable to generate any tokens for {:?}. Aborting.",
                self.file_name
            ));
        }
    }

    fn parse_api_response(&mut self) {
        self.file_name = format!("{}_{}", self.options.project, self.options.environment);
        self.file = self.options.api_envs.to_owned();

        self.parse();
    }

    fn parse_files(&mut self) {
        for env in &self.options.files {
            self.index = 0;
            self.byte = 1;
            self.line = 1;
            self.file_name = env.clone();
            self.file = String::new();
            self.file_path = globals::current_dir().clone();

            if !self.options.dir.is_empty() {
                self.file_path.push(&self.options.dir);
            }

            self.file_path.push(&self.file_name);

            // TODO - Check for a correct .env file extension
            // if !self.file_name.ends_with(".env") {
            //     self.logger.fatal(format!(
            //             "was unable to load \"{}\". This file doesn't appear to have the file correct extension. Expected: \".env\" instead got \"{}\".",
            //             self.file_path.display(),
            //             self.file_name
            //     ));
            // }

            if !self.file_path.is_file() {
                self.logger.fatal(format!(
                    "was unable to locate {:?}. The .env file doesn't appear to exist at this path!",
                    self.file_path.display()
                ));
            }

            match fs::read_to_string(&self.file_path) {
                Ok(c) => {
                    self.file = c;
                }
                Err(err) => {
                    self.logger.fatal(format!(
                        "could not read file: {:?}. Reason: {err}.",
                        self.file_path.display(),
                    ));
                }
            };

            if self.file.is_empty() {
                self.logger.fatal(format!(
                    "tried to load file: {:?}, but it doesn't appear to have any ENVs. Aborting.",
                    self.file_path.display(),
                ));
            }

            self.logger.debug(format!(
                "successfully parsed the file {:?}!",
                self.file_path.display()
            ));

            self.parse();
        }
    }

    pub fn tokenize(&mut self) {
        if self.options.api {
            self.parse_api_response();
        } else {
            self.parse_files();
        }

        self.logger.debug(format!(
            "generated the following tokens...\n{:#?}\n{}",
            self.tokens,
            self.tokens.len()
        ));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::options::Options;
    use std::sync::OnceLock;

    fn get_tokens() -> &'static Vec<LexerToken> {
        static TOKENS: OnceLock<Vec<LexerToken>> = OnceLock::new();
        TOKENS.get_or_init(|| {
            let argv = vec![
                String::from("nvi"),
                String::from("--debug"),
                String::from("--directory"),
                String::from("envs"),
                String::from("--files"),
                String::from("simple.env"),
            ];
            let options = Options::new(argv);

            let mut lexer = Lexer::new(&options);
            lexer.tokenize();

            return lexer.get_tokens();
        })
    }

    fn get_token(index: usize) -> LexerToken {
        let tokens = get_tokens();

        return tokens[index].to_owned();
    }

    #[test]
    fn basic_env() {
        let token = get_token(0);

        assert_eq!(token.key, Some(String::from("BASIC_ENV")));
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 1);

        let vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("true")));
        assert_eq!(vt.line, 1);
        assert_eq!(vt.byte, 15);
    }

    #[test]
    fn no_env() {
        let token = get_token(1);

        assert_eq!(token.key, Some(String::from("NO_VALUE")));
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 1);

        let vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("")));
        assert_eq!(vt.line, 2);
        assert_eq!(vt.byte, 10);
    }

    #[test]
    fn comment() {
        let token = get_token(3);

        assert_eq!(token.key, None);
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 1);

        let vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Comment);
        assert_eq!(vt.value, Some(String::from("# COMMENT")));
        assert_eq!(vt.line, 4);
        assert_eq!(vt.byte, 10);
    }

    #[test]
    fn multiline_env() {
        let token = get_token(4);

        assert_eq!(token.key, Some(String::from("MULTI_LINE_KEY")));
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 5);

        let mut vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("ssh-rsa BBBB")));
        assert_eq!(vt.line, 6);
        assert_eq!(vt.byte, 28);

        vt = token.values[1].to_owned();
        assert_eq!(vt.token_type, LexerValue::Multiline);
        assert_eq!(vt.value, Some(String::from("Pl1P1")));
        assert_eq!(vt.line, 7);
        assert_eq!(vt.byte, 6);

        vt = token.values[2].to_owned();
        assert_eq!(vt.token_type, LexerValue::Multiline);
        assert_eq!(vt.value, Some(String::from("A")));
        assert_eq!(vt.line, 8);
        assert_eq!(vt.byte, 4);

        vt = token.values[3].to_owned();
        assert_eq!(vt.token_type, LexerValue::Multiline);
        assert_eq!(vt.value, Some(String::from("D+jk/3")));
        assert_eq!(vt.line, 9);
        assert_eq!(vt.byte, 9);

        vt = token.values[4].to_owned();
        assert_eq!(vt.token_type, LexerValue::Multiline);
        assert_eq!(vt.value, Some(String::from("Lf3Dw== test@example.com")));
        assert_eq!(vt.line, 10);
        assert_eq!(vt.byte, 27);
    }

    #[test]
    fn interpolated_env() {
        let token = get_token(5);

        assert_eq!(token.key, Some(String::from("INTERP_VALUE")));
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 2);

        let mut vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MESSAGE")));
        assert_eq!(vt.line, 12);
        assert_eq!(vt.byte, 24);

        vt = token.values[1].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("world")));
        assert_eq!(vt.line, 12);
        assert_eq!(vt.byte, 29);
    }

    #[test]
    fn multiple_interpolated_env() {
        let token = get_token(11);

        assert_eq!(token.key, Some(String::from("MONGOLAB_URI")));
        assert_eq!(token.file, String::from("simple.env"));
        assert_eq!(token.values.len(), 10);

        let mut vt = token.values[0].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("mongodb://")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 24);

        vt = token.values[1].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MONGOLAB_USER")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 40);

        vt = token.values[2].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from(":")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 41);

        vt = token.values[3].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MONGOLAB_PASSWORD")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 61);

        vt = token.values[4].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("@")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 62);

        vt = token.values[5].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MONGOLAB_DOMAIN")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 80);

        vt = token.values[6].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from(":")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 81);

        vt = token.values[7].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MONGOLAB_PORT")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 97);

        vt = token.values[8].to_owned();
        assert_eq!(vt.token_type, LexerValue::Normal);
        assert_eq!(vt.value, Some(String::from("/")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 98);

        vt = token.values[9].to_owned();
        assert_eq!(vt.token_type, LexerValue::Interpolated);
        assert_eq!(vt.value, Some(String::from("MONGOLAB_DATABASE")));
        assert_eq!(vt.line, 19);
        assert_eq!(vt.byte, 118);
    }

    #[test]
    fn api_response() {
        let mut options = Options::new(vec![]);
        options.api = true;
        options.api_envs.push_str("API_KEY_1=sdfksdfj\n");
        options.api_envs.push_str("API_KEY_2=${API_KEY_1}\n");
        options.api_envs.push_str("API_KEY_3=ssh-rsa BBBB\\\n");
        options.api_envs.push_str("Pl1P1\\\n");
        options.api_envs.push_str("A\\\n");
        options.api_envs.push_str("D+jk/3\\\n");
        options.api_envs.push_str("Lf3Dw== test@example.com\n");
        options.api_envs.push_str("API_KEY_4='  SINGLE QUOTES  '\n");

        let mut lexer = Lexer::new(&options);
        lexer.tokenize();

        let tokens = lexer.get_tokens();

        assert_eq!(tokens.len(), 4);
    }
}
