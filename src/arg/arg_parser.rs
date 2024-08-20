use super::ARG;
use crate::globals::ARGS;
use crate::logger::Logger;
use crate::options::OptionsType;
use std::env;
use std::fs;
use std::path::PathBuf;
use toml::{Table, Value};

pub struct ArgParser<'a> {
    argc: usize,
    argv: Vec<String>,
    file: PathBuf,
    curr_flag: String,
    options: &'a mut OptionsType,
    index: usize,
    logger: Logger<'a>,
}

impl<'a> ArgParser<'a> {
    pub fn new(options: &'a mut OptionsType) -> Self {
        let argv: Vec<String> = env::args().collect();

        let mut logger = Logger::new("ArgParser");
        logger.set_debug(&options.debug);

        return ArgParser {
            argc: argv.len(),
            argv,
            file: env::current_dir().unwrap_or(PathBuf::new()),
            curr_flag: String::new(),
            options,
            index: 1,
            logger,
        };
    }

    fn get_vec_str_opt(&self, config: &Value, prop: &str) -> Option<Vec<String>> {
        if let Some(opt_vec) = config.get(prop) {
            let values = match opt_vec {
                toml::Value::Array(v) => v,
                _ => self.logger.fatal(format!(
                    "expected the {prop:?} config option to be an array, instead parsed a(n): {}.",
                    opt_vec.type_str()
                )),
            };

            let mut opts = vec![];

            for val in values {
                match val {
                toml::Value::String(v) => opts.push(v.to_owned()),
                _ => self.logger.fatal(format!(
                        "expected the {prop:?} config option to be an array of strings, instead found {val}, which is a(n): {}.",
                        val.type_str()
                    )),
                }
            }

            return Some(opts);
        };

        return None;
    }

    fn get_str_opt(&self, config: &Value, prop: &str) -> Option<String> {
        if let Some(value) = config.get(prop) {
            match value {
                toml::Value::String(v) => return Some(v.to_owned()),
                _ => self.logger.fatal(format!(
                    "expected the {prop:?} config option to be a string value, instead parsed a(n): {}.",
                    value.type_str()
                )),
            };
        };

        return None;
    }

    fn get_bool_opt(&self, config: &Value, prop: &str) -> Option<bool> {
        if let Some(value) = config.get(prop) {
            match value {
                toml::Value::Boolean(v) => return Some(*v),
                _ => self.logger.fatal(format!(
                    "expected the {prop:?} config option to be a boolean value, instead parsed a(n): {}.",
                    value.type_str()
                )),
            };
        };

        return None;
    }

    fn parse_config(&mut self) -> &mut Self {
        if !self.options.dir.is_empty() {
            self.file.push(&self.options.dir);
        }

        self.file.push("nvi.toml");

        let file_contents = match fs::read_to_string(&self.file) {
            Ok(c) => c,
            Err(err) => {
                self.logger.fatal(format!(
                    "could not read file: {:?}. Reason: {err}.",
                    self.file.display(),
                ));
            }
        };

        let parsed_config: Table = match toml::from_str(&file_contents) {
            Ok(pc) => pc,
            Err(err) => {
                self.logger.fatal(format!(
                    "was unable to load the toml config data from {:?}. Reason: {err}.",
                    self.file.display(),
                ));
            }
        };

        let config = match parsed_config.get(&self.options.config) {
            Some(c) => c,
            None => {
                self.logger.fatal(format!(
                    "was unable to load the {:?} environment config from {:?}. Reason: The environment configuration does't appear to exist. Maybe a spelling error?",
                    self.options.config,
                    self.file.display(),
                ));
            }
        };

        self.logger.debug(format!(
            "successfully loaded the {:?} config from {:?} with the following options... \n{:#?}",
            self.options.config,
            self.file.display(),
            config
        ));

        if let Some(api) = self.get_bool_opt(config, "api") {
            self.options.api = api;
        }

        if let Some(debug) = self.get_bool_opt(config, "debug") {
            self.options.debug = debug;
            self.logger.set_debug(&self.options.debug);
        }

        if let Some(directory) = self.get_str_opt(config, "directory") {
            self.options.dir = directory;
        }

        if let Some(environment) = self.get_str_opt(config, "environment") {
            self.options.environment = environment;
        }

        if let Some(execute) = self.get_vec_str_opt(config, "execute") {
            self.options.commands = execute;
        }

        if let Some(files) = self.get_vec_str_opt(config, "files") {
            self.options.files = files;
        }

        if let Some(print) = self.get_bool_opt(config, "print") {
            self.options.print = print;
        }

        if let Some(project) = self.get_str_opt(config, "project") {
            self.options.project = project;
        }

        if let Some(required_envs) = self.get_vec_str_opt(config, "required") {
            self.options.required_envs = required_envs;
        }

        if let Some(save) = self.get_bool_opt(config, "save") {
            self.options.save = save;
        }

        return self;
    }

    fn next_arg(&mut self) {
        self.index += 1;
    }

    fn prev_arg(&mut self) {
        self.index -= 1;
    }

    fn get_arg(&'a self) -> &'a str {
        return match self.argv.get(self.index) {
            Some(a) => a,
            None => "",
        };
    }

    fn parse_single_arg(&mut self, flag: ARG) -> String {
        self.next_arg();

        let arg = self.get_arg();

        if arg.is_empty() || arg.contains("-") {
            // TODO - Add per flag error messages
            self.logger
                .fatal(format!("error {flag} {}", self.curr_flag));
        }

        return arg.to_owned();
    }

    fn parse_multi_arg(&mut self, flag: ARG, delimiter_stop: bool) -> Vec<String> {
        let mut args = vec![];

        while self.index < self.argc {
            self.next_arg();

            let arg = self.get_arg();

            if arg.is_empty() {
                break;
            }

            if delimiter_stop && arg.contains("-") {
                self.prev_arg();
                break;
            }

            args.push(arg.to_owned());
        }

        // TODO - Add per flag error messages
        if args.is_empty() {
            self.logger.fatal(format!("error for {flag}"));
        }

        return args;
    }

    pub fn parse(&mut self) {
        while self.index < self.argc {
            self.curr_flag = match self.argv.get(self.index) {
                Some(f) => f.to_owned(),
                None => ARG::UNKNOWN.to_string(),
            };

            match ARGS.get(self.curr_flag.as_str()) {
                Some(ARG::API) => self.options.api = true,
                Some(ARG::CONFIG) => {
                    self.options.config = self.parse_single_arg(ARG::CONFIG);
                }
                Some(ARG::DEBUG) => {
                    self.options.debug = true;
                    self.logger.set_debug(&self.options.debug);
                }
                Some(ARG::DIRECTORY) => {
                    self.options.dir = self.parse_single_arg(ARG::DIRECTORY);
                }
                Some(ARG::ENVIRONMENT) => {
                    self.options.environment = self.parse_single_arg(ARG::ENVIRONMENT);
                }
                Some(ARG::EXECUTE) => {
                    self.options.commands = self.parse_multi_arg(ARG::EXECUTE, false);
                }
                Some(ARG::FILES) => {
                    self.options.files = self.parse_multi_arg(ARG::FILES, true);
                }
                Some(ARG::HELP) => self.logger.print_help_and_exit(),
                Some(ARG::PRINT) => self.options.print = true,
                Some(ARG::PROJECT) => {
                    self.options.project = self.parse_single_arg(ARG::PROJECT);
                }
                Some(ARG::REQUIRED) => {
                    self.options.required_envs = self.parse_multi_arg(ARG::REQUIRED, true);
                }
                Some(ARG::SAVE) => self.options.save = true,
                Some(ARG::VERSION) => self.logger.print_version_and_exit(),
                _ => {
                    let mut invalid_args = String::new();

                    while self.index < self.argc {
                        self.next_arg();

                        let arg = self.get_arg();

                        if arg.is_empty() || arg.contains("-") {
                            self.prev_arg();
                            break;
                        }

                        if !invalid_args.is_empty() {
                            invalid_args.push_str(" ");
                        }

                        invalid_args.push_str(arg);
                    }

                    let mut message = format!("found an unknown flag {:?}", self.curr_flag);

                    if !invalid_args.is_empty() {
                        message.push_str(format!(" with args {invalid_args:?}").as_str());
                    }

                    self.logger.warn(format!("{message}. Skipping."));
                }
            }

            self.next_arg();
        }

        if !self.options.config.is_empty() {
            self.logger.debug(format!(
                "is attempting to load the {:?} config from the nvi.toml...",
                self.options.config
            ));

            self.parse_config();
        }

        self.logger.debug(format!(
            "has set the following flag options... \n{:#?}",
            self.options
        ));
    }
}
