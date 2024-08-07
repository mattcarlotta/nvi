use super::ArgParser;
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
        command: "{}",
        config: "{}",
        debug: {},
        directory: "{}",
        environment: "{}",
        files: [{}],
        print: {},
        project: "{}",
        required_envs: [{}],
        save: {}"#,
            self.api,
            self.commands.join(" "),
            self.config,
            self.debug,
            self.dir,
            self.environment,
            self.files.join(", "),
            self.print,
            self.project,
            self.required_envs.join(", "),
            self.save
        );
    }
}

impl Options {
    pub fn new() -> Self {
        let mut arg_parser = ArgParser::new();
        arg_parser.parse();

        return arg_parser.get_options();
    }
}
