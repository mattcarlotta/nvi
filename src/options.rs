use std::env;

#[derive(Debug)]
pub struct Options<'a> {
    pub argc: usize,
    pub args: Vec<String>,
    pub api: bool,
    pub api_envs: String,
    pub commands: Vec<&'a str>,
    pub config: String,
    pub debug: bool,
    pub dir: String,
    pub environment: String,
    pub files: Vec<&'a str>,
    pub print: bool,
    pub project: String,
    pub required_envs: Vec<&'a str>,
    pub save: bool,
}

impl<'a> Options<'a> {
    pub fn new() -> Self {
        let args: Vec<String> = env::args().collect();

        Options {
            argc: args.len(),
            args,
            api: false,
            api_envs: String::new(),
            commands: vec![],
            config: String::new(),
            debug: false,
            dir: String::new(),
            environment: String::new(),
            files: vec![".env"],
            print: false,
            project: String::new(),
            required_envs: vec![],
            save: false,
        }
    }
}

pub type OptionsType<'a> = Options<'a>;
