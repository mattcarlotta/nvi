use crate::logger::Logger;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::env;
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

#[derive(Debug)]
pub struct ArgParser<'a> {
    curr_flag: String,
    index: usize,
    logger: Logger<'a>,
}

impl<'a> ArgParser<'a> {
    pub fn new() -> Self {
        return ArgParser {
            curr_flag: String::new(),
            index: 1,
            logger: Logger::new("ArgParser"),
        };
    }
}

#[derive(Debug)]
pub struct Options<'a> {
    arg_parser: ArgParser<'a>,
    pub argc: usize,
    pub argv: Vec<String>,
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

impl<'a> Display for Options<'a> {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        write!(f, "{}", self)
    }
}

impl<'a> Options<'a> {
    pub fn new() -> Self {
        let argv: Vec<String> = env::args().collect();

        let mut options = Options {
            arg_parser: ArgParser::new(),
            argc: argv.len(),
            argv,
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
        };

        options.parse();

        return options;
    }

    fn parse_single_arg(&mut self, flag: ARG) -> Result<String, ()> {
        self.arg_parser.index += 1;
        match self.argv.get(self.arg_parser.index) {
            Some(arg) => {
                if arg.contains("-") {
                    return Err(self
                        .arg_parser
                        .logger
                        .fatal(format!("error {} {}", flag, self.arg_parser.curr_flag)));
                }
                return Ok(String::from(arg));
            }
            None => Err(self
                .arg_parser
                .logger
                .fatal(format!("invalid arg error {}", flag))),
        }
    }

    pub fn parse(&mut self) -> &mut Self {
        while self.arg_parser.index < self.argc {
            self.arg_parser.curr_flag = match self.argv.get(self.arg_parser.index) {
                Some(f) => String::from(f),
                None => String::from(ARG::UNKNOWN.as_str()),
            };

            match ARGS.get(self.arg_parser.curr_flag.as_str()) {
                Some(ARG::API) => self.api = true,
                Some(ARG::CONFIG) => self.config = self.parse_single_arg(ARG::CONFIG).unwrap(),
                Some(ARG::DEBUG) => self.debug = true,
                Some(ARG::DIRECTORY) => {
                    self.dir = self.parse_single_arg(ARG::DIRECTORY).unwrap();
                }
                Some(ARG::ENVIRONMENT) => {
                    self.environment = self.parse_single_arg(ARG::ENVIRONMENT).unwrap();
                }
                Some(ARG::EXECUTE) => println!("{}", ARG::EXECUTE),
                Some(ARG::FILES) => {
                    // self.options.files = self.parse_single_arg(ARG::FILES).unwrap();
                }
                Some(ARG::HELP) => println!("{}", ARG::HELP),
                Some(ARG::PRINT) => self.print = true,
                Some(ARG::PROJECT) => {
                    self.project = self.parse_single_arg(ARG::PROJECT).unwrap();
                }
                Some(ARG::REQUIRED) => println!("{}", ARG::REQUIRED),
                Some(ARG::SAVE) => self.save = true,
                Some(ARG::VERSION) => self.arg_parser.logger.print_version_and_exit(),
                Some(ARG::UNKNOWN) | None => {
                    let mut invalid_flag = String::new();

                    while self.arg_parser.index < self.argc {
                        self.arg_parser.index += 1;

                        match self.argv.get(self.arg_parser.index) {
                            Some(flag) => {
                                if flag.contains("-") {
                                    self.arg_parser.index -= 1;
                                    break;
                                }

                                if invalid_flag.len() > 0 {
                                    invalid_flag += &format!(" {}", flag);
                                } else {
                                    invalid_flag += &flag;
                                }
                            }
                            None => {
                                self.arg_parser.index -= 1;
                                break;
                            }
                        }
                    }

                    let mut message =
                        format!("found an unknown flag: \"{}\"", self.arg_parser.curr_flag);

                    if invalid_flag.len() > 0 {
                        message += &format!(" with args \"{}\"", invalid_flag);
                    }

                    self.arg_parser.logger.warn(message + ". Skipping.");
                }
            }

            self.arg_parser.index += 1;
        }

        if self.debug {
            let message = format!("set the following options:\n  {:?}", self);
            self.arg_parser.logger.debug(message);
        }

        return self;
    }
}
