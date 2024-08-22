use crate::api::Api;
use crate::arg::ArgParser;
use std::fmt::{Debug, Formatter, Result as FmtResult};

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

impl Debug for Options {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        return write!(
            f,
            "{{\n    \"api\": {},\n    \"command\": {:?},\n    \"config\": {:?},\n    \"debug\": {},\n    \"directory\": {:?},\n    \"environment\": {:?},\n    \"files\": {:?},\n    \"print\": {},\n    \"project\": {:?},\n    \"required_envs\": {:?},\n    \"save\": {}\n}}",
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
    pub fn new(argv: Vec<String>) -> Self {
        let mut options = Options {
            api: false,
            api_envs: String::new(),
            commands: vec![],
            config: String::new(),
            debug: false,
            dir: String::new(),
            environment: String::new(),
            files: vec![String::from(".env")],
            print: false,
            project: String::new(),
            required_envs: vec![],
            save: false,
        };

        {
            let mut arg_parser = ArgParser::new(&mut options, argv);
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_arg_options() {
        let argv = vec![
            String::from("nvi"),
            String::from("--debug"),
            String::from("--directory"),
            String::from("tests"),
            String::from("--environment"),
            String::from("testing"),
            String::from("--files"),
            String::from("test.env"),
            String::from("test2.env"),
            String::from("test3.env"),
            String::from("--project"),
            String::from("rust"),
            String::from("--required"),
            String::from("ENV_KEY_1"),
            String::from("ENV_KEY_2"),
            String::from("ENV_KEY_3"),
            String::from("--save"),
            String::from("--"),
            String::from("echo"),
            String::from("Hello World"),
        ];
        let options = Options::new(argv);

        assert_eq!(options.api, false);
        assert_eq!(options.api_envs, String::new());
        assert_eq!(options.commands[0], String::from("echo"));
        assert_eq!(options.commands[1], String::from("Hello World"));
        assert_eq!(options.debug, true);
        assert_eq!(options.dir, String::from("tests"));
        assert_eq!(options.environment, String::from("testing"));
        assert_eq!(options.files[0], String::from("test.env"));
        assert_eq!(options.files[1], String::from("test2.env"));
        assert_eq!(options.files[2], String::from("test3.env"));
        assert_eq!(options.project, String::from("rust"));
        assert_eq!(options.required_envs[0], String::from("ENV_KEY_1"));
        assert_eq!(options.required_envs[1], String::from("ENV_KEY_2"));
        assert_eq!(options.required_envs[2], String::from("ENV_KEY_3"));
        assert_eq!(options.save, true);
    }
}
