use crate::lexer::{LexerToken, LexerValue};
use crate::logger::Logger;
use crate::options::OptionsType;
use std::collections::HashMap;
use std::env;

pub struct Parser<'a> {
    envs: HashMap<String, String>,
    options: &'a OptionsType,
    tokens: Vec<LexerToken>,
    undefined_keys: Vec<String>,
    logger: Logger<'a>,
}

impl<'a> Parser<'a> {
    pub fn new(options: &'a OptionsType, tokens: Vec<LexerToken>) -> Self {
        let mut logger = Logger::new("Parser");
        logger.set_debug(&options.debug);

        Parser {
            envs: HashMap::new(),
            options,
            tokens,
            undefined_keys: options.required_envs.clone(),
            logger,
        }
    }

    pub fn get_envs(self) -> HashMap<String, String> {
        self.envs
    }

    pub fn parse_tokens(&mut self) {
        for token in &self.tokens {
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
                                        key, interpolated_key, token.file, value_token.line, value_token.byte
                                    )
                                );
                            }
                        }
                        LexerValue::Comment => {
                            if let Some(val) = &value_token.value {
                                self.logger.debug(format!(
                                    "parsed the following comment {:?} ({}:{}:{}). Skipping.",
                                    val, token.file, value_token.line, value_token.byte
                                ));
                            }
                        }
                        _ => {
                            if let Some(val) = &value_token.value {
                                value.push_str(val.as_str());
                            }
                        }
                    }
                }

                if !key.is_empty() {
                    self.envs.insert(key.clone(), value);
                    if !self.undefined_keys.is_empty() {
                        self.undefined_keys.retain(|k| *k != key);
                    }
                }
            }
        }

        if self.envs.is_empty() {
            self.logger.fatal(String::from(
                "Unable to parse any ENVs! Please ensure the provided .env files are not empty.)",
            ));
        }

        if !self.undefined_keys.is_empty() {
            self.logger.fatal(
                format!(
                    "found that the following ENVs were marked as required: {:?}, but they are undefined after all of the ENVs were parsed. Perhaps you forgot to include them in an .env file or an API project's environment?", 
                    self.undefined_keys
                )
             );
        }

        self.logger.debug(format!(
            "generated the following env map...\n{:#?}",
            self.envs
        ));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::Lexer;
    use crate::options::Options;
    use std::process::{Command, Stdio};
    use std::sync::OnceLock;

    fn get_envs() -> &'static HashMap<String, String> {
        static ENVS: OnceLock<HashMap<String, String>> = OnceLock::new();
        ENVS.get_or_init(|| {
            let argv = vec![
                String::from("nvi"),
                String::from("--debug"),
                String::from("--directory"),
                String::from("envs"),
                String::from("--files"),
                String::from(".env"),
                String::from("base.env"),
                String::from("reference.env"),
            ];
            let options = Options::new(argv);

            let mut lexer = Lexer::new(&options);
            lexer.tokenize();

            let mut parser = Parser::new(&options, lexer.get_tokens());
            parser.parse_tokens();

            parser.get_envs()
        })
    }

    fn get_env(key: &str) -> String {
        let envs = get_envs();

        match envs.get(key) {
            Some(v) => String::from(v),
            None => String::new(),
        }
    }

    #[test]
    fn basic_env() {
        assert_eq!(get_env("BASIC_ENV"), String::from("true"));
    }

    #[test]
    fn base_env() {
        assert_eq!(get_env("BASE"), String::from("hello"));
    }

    #[test]
    fn ref_base_env() {
        assert_eq!(get_env("REFERENCE"), String::from("hello"));
    }

    #[test]
    fn multiline_env() {
        assert_eq!(
            get_env("MULTI_LINE_KEY"),
            String::from("ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com")
        );
    }

    #[test]
    fn no_env() {
        assert_eq!(get_env("NO_VALUE"), String::from(""));
    }

    #[test]
    fn hash_env() {
        assert_eq!(get_env("HASH_KEYS"), String::from("1#2#3#4#"));
    }

    #[test]
    fn dollars_env() {
        assert_eq!(get_env("JUST_DOLLARS"), String::from("$$$$$"));
    }

    #[test]
    fn braces_env() {
        assert_eq!(get_env("JUST_BRACES"), String::from("{{{{}}}}"));
    }

    #[test]
    fn spaces_env() {
        assert_eq!(get_env("JUST_SPACES"), String::from("          "));
    }

    #[test]
    fn sentence_env() {
        assert_eq!(
            get_env("SENTENCE"),
            String::from(
                "chat gippity is just a junior engineer that copies/pastes from StackOverflow"
            )
        );
    }

    #[test]
    fn interpolate_from_system_env() {
        assert_eq!(get_env("INTERP_ENV_FROM_SYS").len() > 0, true);
    }

    #[test]
    fn empty_interpolated_env() {
        assert_eq!(get_env("EMPTY_INTERP_KEY"), String::from("abc123"));
    }

    #[test]
    fn single_quotes_env() {
        assert_eq!(
            get_env("SINGLE_QUOTES_SPACES"),
            String::from("'  SINGLE QUOTES  '")
        );
    }

    #[test]
    fn double_quotes_env() {
        assert_eq!(
            get_env("DOUBLE_QUOTES_SPACES"),
            String::from("\"  DOUBLE QUOTES  \"")
        );
    }

    #[test]
    fn quotes_env() {
        assert_eq!(get_env("QUOTES"), String::from("sad\"wow\"bak"));
    }

    #[test]
    fn invalid_single_quotes_env() {
        assert_eq!(get_env("INVALID_SINGLE_QUOTES"), String::from("bad\'dq"));
    }

    #[test]
    fn invalid_double_quotes_env() {
        assert_eq!(get_env("INVALID_DOUBLE_QUOTES"), String::from("bad\"dq"));
    }

    #[test]
    fn invalid_template_literal_env() {
        assert_eq!(get_env("INVALID_TEMPLATE_LITERAL"), String::from("bad`as"));
    }

    #[test]
    fn invalid_mix_quotes_env() {
        assert_eq!(get_env("INVALID_MIX_QUOTES"), String::from("bad\"'`mq"));
    }

    #[test]
    fn empty_single_quotes_env() {
        assert_eq!(get_env("EMPTY_SINGLE_QUOTES"), String::from("''"));
    }

    #[test]
    fn empty_double_quotes_env() {
        assert_eq!(get_env("EMPTY_DOUBLE_QUOTES"), String::from("\"\""));
    }

    #[test]
    fn empty_template_literal_env() {
        assert_eq!(get_env("EMPTY_TEMPLATE_LITERAL"), String::from("``"));
    }

    #[test]
    fn multiple_interpolated_env() {
        assert_eq!(
            get_env("MONGOLAB_URI"),
            String::from("mongodb://root:password@abcd1234.mongolab.com:12345/localhost")
        );
    }

    #[test]
    fn prints_empty_env_error() {
        match Command::new("./target/debug/nvi")
            .args(["--directory", "envs", "--files", "comment.env"])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("Unable to parse any ENVs! Please ensure the provided .env files are not empty."),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_empty_env_error test. Reason: {err}");
            }
        }
    }

    #[test]
    fn prints_required_env_error() {
        match Command::new("./target/debug/nvi")
            .args([
                "--directory",
                "envs",
                "--files",
                "base.env",
                "--required",
                "KEY1",
            ])
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
        {
            Ok(c) => {
                let output = c.wait_with_output().expect("Failed to read command output");

                assert!(!output.status.success());

                let stderr = String::from_utf8_lossy(&output.stderr);

                assert!(
                    stderr.contains("found that the following ENVs were marked as required: [\"KEY1\"], but they are undefined"),
                );
            }
            Err(err) => {
                panic!("Failed to run prints_required_env_error test. Reason: {err}");
            }
        }
    }
}
