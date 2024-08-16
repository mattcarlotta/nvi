use super::{LexerToken, LexerValue, LexerValueToken};
use crate::globals;
use crate::logger::Logger;
use crate::options::OptionsType;
use std::env;
use std::fs;
use std::path::PathBuf;

pub struct Lexer<'a> {
    options: &'a OptionsType,
    index: usize,
    byte: usize,
    line: usize,
    // envs: String,
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
            // envs: String::new(),
            file: String::new(),
            file_name: String::new(),
            file_path: env::current_dir().unwrap_or(PathBuf::new()),
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
        self.index += offset.unwrap_or(1);
    }

    fn get_char(&self) -> char {
        return match self.peek(None) {
            Some(c) => c,
            None => globals::NULL_CHAR,
        };
    }

    fn get_next_char(&self) -> char {
        return match self.peek(Some(1)) {
            Some(c) => c,
            None => globals::NULL_CHAR,
        };
    }

    fn parse_file(&mut self) {
        // let mut token = LexerToken::new();
        // token.file = self.file_name.to_owned();

        let mut value = String::new();
        let mut key = String::new();
        while self.peek(None).is_some() {
            let current_char = self.get_char();

            if !key.is_empty() || !value.is_empty() {
                println!("key: {key}, value: {value}");
            }

            if current_char == globals::NULL_CHAR || current_char == globals::LINE_DELIMITER {
                value.clear();
                key.clear();
                self.skip(None);
                continue;
            }
            println!(
                "char: {}, line: {}, byte: {}, index: {}",
                current_char.escape_default().to_string(),
                self.line,
                self.byte,
                self.index,
            );

            if current_char.is_alphanumeric() {
                while self.peek(None).is_some() && self.get_char().is_alphanumeric() {
                    value.push(self.get_char());
                    self.skip(None);
                }
                continue;
            } else if current_char.is_whitespace() || current_char.is_ascii_punctuation() {
                match current_char {
                    globals::LINE_DELIMITER => continue,
                    globals::ASSIGN_OP => {
                        key = value.to_owned();
                        self.inc_byte(None);
                        value.clear();

                        // self.skip '='
                        self.skip(None);
                        continue;
                    }
                    globals::HASH => {
                        if !key.is_empty() {
                            // commit lines with hashes
                            value.push(current_char);
                            self.skip(None);
                            continue;
                        }

                        // skip lines with comments
                        while self.peek(None).is_some()
                            && self.get_char() != globals::LINE_DELIMITER
                        {
                            self.skip(None);
                        }

                        self.inc_line();
                        // self.skip '\n'
                        self.skip(None);

                        continue;
                    }
                    globals::DOLLAR_SIGN => {
                        if self.get_next_char() != globals::OPEN_BRACE {
                            value.push(current_char);
                            continue;
                        }

                        if !value.is_empty() {
                            // token.values.push(LexerValueToken {
                            //     token_type: LexerValue::Normal,
                            //     value: Some(value.clone()),
                            //     byte: self.byte,
                            //     line: self.line,
                            // });
                            value.clear();
                        }

                        // self.skip "${"
                        self.skip(Some(2));
                        self.inc_byte(Some(2));

                        while self.peek(None).is_some() && self.get_char() != globals::CLOSE_BRACE {
                            if self.get_char() == globals::LINE_DELIMITER {
                                // _token_key = token.key.value();
                                // logger.fatal(INTERPOLATION_ERROR);
                            } else {
                                if let Some(c) = self.peek(None) {
                                    value.push(c);
                                }
                                self.skip(None);
                                // value.push_back(commit());
                            }
                        }

                        // token.values.push(LexerValueToken {
                        //     token_type: LexerValue::Interpolated,
                        //     value: Some(value.clone()),
                        //     byte: self.byte,
                        //     line: self.line,
                        // });
                        value.clear();

                        // self.skip '}'
                        self.skip(None);
                        self.inc_byte(None);

                        // if the next value is a new line, store the token and then reset it
                        if self.peek(None).is_some() && self.get_char() == globals::LINE_DELIMITER {
                            // self.tokens.push(token);
                            // token.key = None;
                            // token.values.clear();
                            self.reset_byte();
                            value.clear();
                            self.inc_line();

                            // self.skip '\n'
                            self.skip(None);
                        }
                        continue;
                    }
                    globals::BACK_SLASH => {
                        if self.get_next_char() != globals::LINE_DELIMITER {
                            value.push(current_char);
                            continue;
                        }

                        if !value.is_empty() {
                            // token.values.push({ValueType::normal, value, _byte, _line});
                        }

                        // self.skip "\\n"
                        self.skip(Some(2));
                        self.reset_byte();

                        // handle multiline values
                        value.clear();
                        while self.peek(None).is_some() {
                            let is_eol = self.get_char() == globals::LINE_DELIMITER;

                            if (self.get_char() == globals::BACK_SLASH
                                && self.get_next_char() == globals::LINE_DELIMITER)
                                || is_eol
                            {
                                self.inc_line();

                                // TODO: Fix this WET code
                                // if is_eol {
                                //     token.values.push(LexerValueToken {
                                //         token_type: LexerValue::Normal,
                                //         value: Some(value.clone()),
                                //         byte: self.byte,
                                //         line: self.line,
                                //     });
                                // } else {
                                //     token.values.push(LexerValueToken {
                                //         token_type: LexerValue::Multiline,
                                //         value: Some(value.clone()),
                                //         byte: self.byte,
                                //         line: self.line,
                                //     });
                                // }

                                value.clear();
                                self.reset_byte();

                                // self.skip '\n' or "\\n"
                                let mut next_byte: usize = 1;
                                if is_eol {
                                    next_byte += 1;
                                }
                                self.skip(Some(next_byte));

                                if is_eol {
                                    break;
                                }

                                continue;
                            }

                            if let Some(c) = self.peek(None) {
                                value.push(c);
                            }
                            self.skip(None);
                        }

                        // self.tokens.push(token);
                        // token.key = None;
                        // token.values = vec![];
                        value.clear();
                        self.reset_byte();
                        self.inc_line();
                        continue;
                    }
                    _ => {
                        value.push(current_char);
                        self.skip(None);
                        continue;
                    }
                }
            } else {
                // if token.key.is_some() {
                //     token.values.push(LexerValueToken {
                //         token_type: LexerValue::Normal,
                //         value: Some(value.clone()),
                //         byte: self.byte,
                //         line: self.line,
                //     });
                //     self.tokens.push(token);
                // }

                // token.key = None;
                // token.values.clear();
                value.clear();
                self.reset_byte();
                self.inc_line();
                self.skip(None);
                continue;
            }
        }
    }

    pub fn parse_api_response(&mut self) {
        self.index = 0;
        self.byte = 1;
        self.line = 1;
        self.file = self.options.api_envs.to_owned();
        self.file_path = env::current_dir().unwrap_or(PathBuf::new());

        self.parse_file();

        self.logger.debug(format!("lexer tokens"));
    }

    pub fn parse_files(&mut self) {
        for env in &self.options.files {
            self.index = 0;
            self.byte = 1;
            self.line = 1;
            self.file_name = env.clone();
            self.file = String::new();
            self.file_path = env::current_dir().unwrap_or(PathBuf::new());

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
                    "was unable to locate \"{}\". The .env file doesn't appear to exist at this path!",
                    self.file_path.display()
                ));
            }

            match fs::read_to_string(&self.file_path) {
                Ok(c) => {
                    self.file = c;
                }
                Err(err) => {
                    self.logger.fatal(format!(
                        "could not read file: \"{}\". Reason: {err}.",
                        self.file_path.display(),
                    ));
                }
            };

            if self.file.is_empty() {
                self.logger.fatal(format!(
                    "tried to load file: \"{}\", but it doesn't appear to have any ENVs. Aborting.",
                    self.file_path.display(),
                ));
            }

            self.logger.debug(format!(
                "successfully parsed the file \"{}\"!",
                self.file_path.display()
            ));

            self.parse_file();
        }

        // if (_options.debug) {
        //     logger.debug(DEBUG_LEXER);
        // }

        // return this;
    }
}
