use crate::logger::Logger;
use crate::options::OptionsType;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::fmt::{Display, Formatter, Result as FmtResult};

enum ARG {
    API,
    CONFIG,
    DEBUG,
    DIRECTORY,
    ENVIRONMENT,
    EXECUTE,
    FILES,
    HELP,
    PRINT,
    PROJECT,
    REQUIRED,
    SAVE,
    VERSION,
    UNKNOWN,
}

// pub type ARG_T = ARG;

impl ARG {
    fn as_str(&self) -> &'static str {
        match self {
            ARG::API => "--api",
            ARG::CONFIG => "--config",
            ARG::DEBUG => "--debug",
            ARG::DIRECTORY => "--directory",
            ARG::ENVIRONMENT => "--environment",
            ARG::EXECUTE => "--execute",
            ARG::FILES => "--files",
            ARG::HELP => "--help",
            ARG::PRINT => "--print",
            ARG::PROJECT => "--project",
            ARG::REQUIRED => "--required",
            ARG::SAVE => "--save",
            ARG::VERSION => "--version",
            ARG::UNKNOWN => "",
        }
    }

    fn debug(&self) -> &'static str {
        match self {
            ARG::API => "API",
            ARG::CONFIG => "CONFIG",
            ARG::DEBUG => "DEBUG",
            ARG::DIRECTORY => "DIRECTORY",
            ARG::ENVIRONMENT => "ENVIRONMENT",
            ARG::EXECUTE => "EXECUTE",
            ARG::FILES => "FILES",
            ARG::HELP => "HELP",
            ARG::PRINT => "PRINT",
            ARG::PROJECT => "PROJECT",
            ARG::REQUIRED => "REQUIRED",
            ARG::SAVE => "SAVE",
            ARG::VERSION => "VERSION",
            ARG::UNKNOWN => "UNKNOWN",
        }
    }
}

impl Display for ARG {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        write!(f, "{}", self.debug())
    }
}

lazy_static! {
    static ref ARGS: HashMap<&'static str, ARG> = {
        return HashMap::from([
            (ARG::API.as_str(), ARG::API),
            (ARG::CONFIG.as_str(), ARG::CONFIG),
            (ARG::DEBUG.as_str(), ARG::DEBUG),
            (ARG::DIRECTORY.as_str(), ARG::DIRECTORY),
            (ARG::ENVIRONMENT.as_str(), ARG::ENVIRONMENT),
            (ARG::EXECUTE.as_str(), ARG::EXECUTE),
            (ARG::FILES.as_str(), ARG::FILES),
            (ARG::HELP.as_str(), ARG::HELP),
            (ARG::PRINT.as_str(), ARG::PRINT),
            (ARG::PROJECT.as_str(), ARG::PROJECT),
            (ARG::REQUIRED.as_str(), ARG::REQUIRED),
            (ARG::SAVE.as_str(), ARG::SAVE),
            (ARG::VERSION.as_str(), ARG::VERSION),
            (ARG::UNKNOWN.as_str(), ARG::UNKNOWN),
        ]);
    };
}

pub struct ArgParser<'a> {
    index: usize,
    options: &'a mut OptionsType<'a>,
    logger: Logger<'a>,
}

impl<'a> ArgParser<'a> {
    pub fn new(options: &'a mut OptionsType<'a>) -> Self {
        return ArgParser {
            index: 1,
            options,
            logger: Logger::new("ArgParser"),
        };
    }

    fn parse_single_arg(&mut self, flag: ARG) -> Result<String, ()> {
        let curr_arg = match self.options.argv.get(self.index) {
            Some(f) => f,
            None => ARG::UNKNOWN.as_str(),
        };
        self.index += 1;
        match self.options.argv.get(self.index) {
            Some(arg) => {
                if arg.contains("-") {
                    return Err(self.logger.fatal(format!("error {} {}", flag, curr_arg)));
                }
                return Ok(String::from(arg));
            }
            None => Err(self.logger.fatal(format!("invalid arg error {}", flag))),
        }
    }

    pub fn parse(&mut self) {
        while self.index < self.options.argc {
            let flag = match self.options.argv.get(self.index) {
                Some(f) => f,
                None => ARG::UNKNOWN.as_str(),
            };

            match ARGS.get(flag) {
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
                Some(ARG::EXECUTE) => println!("{}", ARG::EXECUTE),
                Some(ARG::FILES) => {
                    // self.options.files = self.parse_single_arg(ARG::FILES).unwrap();
                }
                Some(ARG::HELP) => println!("{}", ARG::HELP),
                Some(ARG::PRINT) => self.options.print = true,
                Some(ARG::PROJECT) => {
                    self.options.project = self.parse_single_arg(ARG::PROJECT).unwrap();
                }
                Some(ARG::REQUIRED) => println!("{}", ARG::REQUIRED),
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

                    let mut message = format!("found an unknown flag: \"{}\"", flag);

                    if invalid_flag.len() > 0 {
                        message += &format!(" with args \"{}\"", invalid_flag);
                    }

                    self.logger.warn(message + ". Skipping.");
                }
            }

            self.index += 1;
        }

        if self.options.debug {
            let message = format!("set the following options:\n  {:?}", self.options);
            self.logger.debug(message);
        }
    }
}
