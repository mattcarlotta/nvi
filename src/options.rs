use crate::api::Api;
use crate::arg::ArgParser;
// use std::fmt::{Debug, Formatter, Result as FormatResult};

#[derive(Debug)]
pub struct Options {
    pub api: bool,
    pub api_envs: String,
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

// impl Debug for Options {
//     fn fmt(&self, f: &mut Formatter) -> FormatResult {
//         return write!(
//             f,
//             "api: {}, command: {:?}, config: \"{}\", debug: {}, directory: \"{}\", environment: \"{}\", files: {:?}, print: {}, project: \"{}\", required_envs: {:?}, save: {}",
//             self.api,
//             self.commands,
//             self.config,
//             self.debug,
//             self.dir,
//             self.environment,
//             self.files,
//             self.print,
//             self.project,
//             self.required_envs,
//             self.save
//         );
//     }
// }

impl Options {
    pub fn new() -> Self {
        let mut options = Options {
            api: false,
            api_envs: String::new(),
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

        {
            if options.api {
                let mut api = Api::new(&mut options);

                api.get_and_set_api_envs();
            }
        }

        return options;
    }
}
