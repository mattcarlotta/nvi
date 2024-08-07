use super::{Options, OptionsType, ARG, ARGS};
use crate::logger::Logger;
use std::env;

#[derive(Debug)]
pub struct ArgParser<'a> {
    curr_flag: String,
    options: OptionsType,
    index: usize,
    logger: Logger<'a>,
}

impl<'a> ArgParser<'a> {
    pub fn new() -> Self {
        let argv: Vec<String> = env::args().collect();

        return ArgParser {
            curr_flag: String::new(),
            options: Options {
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
            },
            index: 1,
            logger: Logger::new("ArgParser"),
        };
    }

    pub fn get_options(self) -> OptionsType {
        return self.options;
    }

    fn parse_single_arg(&mut self, flag: ARG) -> Result<String, ()> {
        self.index += 1;
        let arg = match self.options.argv.get(self.index) {
            Some(a) => a,
            None => "",
        };

        if arg.len() == 0 || arg.contains("-") {
            return Err(self
                .logger
                .fatal(format!("error {} {}", flag, self.curr_flag)));
        }

        return Ok(String::from(arg));
    }

    fn parse_multi_arg(&mut self, flag: ARG, delimiter_stop: bool) -> Result<Vec<String>, ()> {
        let mut args = vec![];

        while self.index < self.options.argc {
            self.index += 1;

            let arg = match self.options.argv.get(self.index) {
                Some(a) => a.to_string(),
                None => "".to_string(),
            };

            if arg.len() == 0 {
                break;
            }

            if delimiter_stop & arg.contains("-") {
                self.index -= 1;
                break;
            }

            args.push(arg);
        }

        if args.len() == 0 {
            self.logger.fatal(format!("error for {}", flag));
        }

        return Ok(args);
    }

    pub fn parse(&mut self) -> &mut Self {
        while self.index < self.options.argc {
            self.curr_flag = match self.options.argv.get(self.index) {
                Some(f) => String::from(f),
                None => String::from(ARG::UNKNOWN.as_str()),
            };

            match ARGS.get(self.curr_flag.as_str()) {
                Some(ARG::API) => self.options.api = true,
                Some(ARG::CONFIG) => {
                    self.options.config = self.parse_single_arg(ARG::CONFIG).unwrap()
                }
                Some(ARG::DEBUG) => self.options.debug = true,
                Some(ARG::DIRECTORY) => {
                    self.options.dir = self.parse_single_arg(ARG::DIRECTORY).unwrap();
                }
                Some(ARG::ENVIRONMENT) => {
                    self.options.environment = self.parse_single_arg(ARG::ENVIRONMENT).unwrap();
                }
                Some(ARG::EXECUTE) => {
                    self.options.commands = self.parse_multi_arg(ARG::EXECUTE, false).unwrap();
                }
                Some(ARG::FILES) => {
                    self.options.files = self.parse_multi_arg(ARG::FILES, true).unwrap();
                }
                Some(ARG::HELP) => self.logger.print_help_and_exit(),
                Some(ARG::PRINT) => self.options.print = true,
                Some(ARG::PROJECT) => {
                    self.options.project = self.parse_single_arg(ARG::PROJECT).unwrap();
                }
                Some(ARG::REQUIRED) => {
                    self.options.required_envs = self.parse_multi_arg(ARG::REQUIRED, true).unwrap();
                }
                Some(ARG::SAVE) => self.options.save = true,
                Some(ARG::VERSION) => self.logger.print_version_and_exit(),
                Some(ARG::UNKNOWN) | None => {
                    let mut invalid_flag = String::new();

                    while self.index < self.options.argc {
                        self.index += 1;

                        match self.options.argv.get(self.index) {
                            Some(flag) => {
                                if flag.contains("-") {
                                    self.index -= 1;
                                    break;
                                }

                                if invalid_flag.len() > 0 {
                                    invalid_flag += &format!(" {}", flag);
                                } else {
                                    invalid_flag += &flag;
                                }
                            }
                            None => {
                                self.index -= 1;
                                break;
                            }
                        }
                    }

                    let mut message = format!("found an unknown flag: \"{}\"", self.curr_flag);

                    if invalid_flag.len() > 0 {
                        message += &format!(" with args \"{}\"", invalid_flag);
                    }

                    self.logger.warn(message + ". Skipping.");
                }
            }

            self.index += 1;
        }

        if self.options.debug {
            let message = format!("set the following flag options...{:?}", self.options);
            self.logger.debug(message);
        }

        return self;
    }
}
