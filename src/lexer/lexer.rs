use super::LexerToken;
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

    // fn reset_byte(&mut self) {
    //     self.byte = 1;
    // }

    fn inc_byte(&mut self, offset: Option<usize>) {
        self.byte += offset.unwrap_or(1);
    }

    // fn inc_line(&mut self) {
    //     self.line += 1;
    // }

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

    fn commit(&mut self) -> Option<char> {
        self.inc_byte(None);
        self.index += 1;

        return self.peek(None);
    }

    fn parse_file(&mut self) {
        let mut token = LexerToken::new();
        token.file = self.file_name.to_owned();

        // let mut value = String::new();
        while self.peek(None).is_some() {
            println!(
                "index: {}, byte: {}, char: {}",
                self.index,
                self.byte,
                self.peek(None).unwrap_or('\0')
            );
            self.commit();
        }
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
