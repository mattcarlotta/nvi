use crate::arg::ArgParser;

#[derive(Debug)]
pub struct Options {
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

impl Options {
    pub fn new() -> Self {
        let mut options = Options {
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
