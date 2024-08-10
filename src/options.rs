use crate::arg::ArgParser;
use std::env;
use std::fmt::{Debug, Formatter, Result as FormatResult};

pub struct Options {
    pub argc: usize,
    pub argv: Vec<String>,
    pub api: bool,
    pub commands: Vec<String>,
    pub config: String,
    pub debug: bool,
    pub dir: String,
    pub environment: String,
    pub files: Vec<String>,
    pub print: bool,
    pub project: String,
    pub required_envs: Vec<String>,
    pub save: bool,
}

pub type OptionsType = Options;

impl Debug for Options {
    fn fmt(&self, f: &mut Formatter) -> FormatResult {
        return write!(
            f,
            r#"     
        api: {},
        command: {:?},
        config: "{}",
        debug: {},
        directory: "{}",
        environment: "{}",
        files: {:?},
        print: {},
        project: "{}",
        required_envs: {:?},
        save: {}"#,
            self.api,
            self.commands,
            self.config,
            self.debug,
            self.dir,
            self.environment,
            self.files,
            self.print,
            self.project,
            self.required_envs,
            self.save
        );
    }
}

impl Options {
    pub fn new() -> Self {
        let argv: Vec<String> = env::args().collect();
        let mut options = Options {
            argc: argv.len(),
            argv,
            api: false,
            commands: vec![],
            config: String::new(),
            debug: false,
            dir: String::new(),
            environment: String::new(),
            files: vec![".env".to_string()],
            print: false,
            project: String::new(),
            required_envs: vec![],
            save: false,
        };

        {
            let mut arg_parser = ArgParser::new(&mut options);
            arg_parser.parse();
        }

        return options;
    }
}
