use crate::lexer::{LexerToken, LexerValue};
use crate::logger::Logger;
use crate::options::OptionsType;
use std::collections::HashMap;
use std::env;

pub struct Parser<'a> {
    envs: HashMap<String, String>,
    options: &'a OptionsType,
    tokens: &'a Vec<LexerToken>,
    undefined_keys: Vec<&'a str>,
    logger: Logger<'a>,
}

impl<'a> Parser<'a> {
    pub fn new(options: &'a OptionsType, tokens: &'a Vec<LexerToken>) -> Self {
        let mut logger = Logger::new("Parser");
        logger.set_debug(&options.debug);

        return Parser {
            envs: HashMap::new(),
            options,
            tokens,
            undefined_keys: vec![],
            logger,
        };
    }

    pub fn get_envs(self) -> HashMap<String, String> {
        return self.envs;
    }

    pub fn parse_tokens(&mut self) {
        for token in self.tokens {
            let mut value = String::new();
            let mut key = String::new();

            if !token.values.is_empty() {
                for value_token in &token.values {
                    if let Some(k) = &token.key {
                        key = String::from(k);
                    }
                    match value_token.token_type {
                        LexerValue::Interpolated => {
                            let mut interpolated_key = String::new();
                            if let Some(interpolated_value) = &value_token.value {
                                interpolated_key.push_str(interpolated_value);
                            }

                            if let Ok(env_val) = env::var(&interpolated_key) {
                                value.push_str(env_val.as_str());
                            } else if let Some(env_map_val) = self.envs.get(&interpolated_key) {
                                value.push_str(env_map_val);
                            } else if self.options.debug {
                                self.logger.warn(
                                    format!(
                                        "found a key {:?} that contains an invalid interpolated variable: {:?} ({}:{}:{}). Unable to locate a value that corresponds to this key. Skipping.",
                                        key, 
                                        interpolated_key,
                                        token.file, 
                                        value_token.line, 
                                        value_token.byte, 
                                    )
                                );
                            }
                        }
                        LexerValue::Comment => {
                            if let Some(val) = &value_token.value {
                                self.logger.debug(
                                    format!(
                                        "parsed the following comment {:?} ({}:{}:{}). Skipping.",
                                        val,
                                        token.file, 
                                        value_token.line, 
                                        value_token.byte, 
                                    )
                                );
                            }
                        },
                        _ => {
                            if let Some(val) = &value_token.value {
                                value.push_str(val.as_str());
                            }
                        }
                    }
                }

                if !key.is_empty() {
                    self.envs.insert(key, value);
                }
            }
        }

        if self.envs.is_empty() {
            self.logger.fatal(String::from(
                "Unable to parse any ENVs! Please ensure the provided .env files are not empty.)",
            ));
        } 

        if !&self.options.required_envs.is_empty() {
            for key in &self.options.required_envs {
                if !self.envs.contains_key(key) {
                    self.undefined_keys.push(key.as_str());
                }
            }
        }

        if !self.undefined_keys.is_empty() {
            self.logger.fatal(
                format!(
                    "found that the following ENVs were marked as required: {:?}, but they are undefined after all of the ENVs were parsed. Perhaps you forgot to include them in an .env file or an API project's environment?", 
                    self.undefined_keys
                )
             );
        }


        self.logger.debug(format!("generated the following env map...\n{:#?}", self.envs));
    }
}
