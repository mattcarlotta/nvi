use crate::logger::Logger;
use crate::options::OptionsType;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::fmt;

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

impl fmt::Display for ARG {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
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

pub fn parse<'a>(options: &mut OptionsType<'a>) {
    let mut i: usize = 1;

    while i < options.argc {
        let flag = match options.args.get(i) {
            Some(f) => f,
            None => ARG::UNKNOWN.as_str(),
        };

        match ARGS.get(flag) {
            Some(ARG::API) => options.api = true,
            Some(ARG::CONFIG) => println!("{}", ARG::CONFIG),
            Some(ARG::DEBUG) => options.debug = true,
            Some(ARG::DIRECTORY) => println!("{}", ARG::DIRECTORY),
            Some(ARG::ENVIRONMENT) => println!("{}", ARG::ENVIRONMENT),
            Some(ARG::EXECUTE) => println!("{}", ARG::EXECUTE),
            Some(ARG::FILES) => println!("{}", ARG::FILES),
            Some(ARG::HELP) => println!("{}", ARG::HELP),
            Some(ARG::PRINT) => options.print = true,
            Some(ARG::PROJECT) => println!("{}", ARG::PROJECT),
            Some(ARG::REQUIRED) => println!("{}", ARG::REQUIRED),
            Some(ARG::SAVE) => options.save = true,
            Some(ARG::VERSION) => Logger::print_version_and_exit(),
            Some(ARG::UNKNOWN) | None => println!("{} {}", ARG::UNKNOWN, flag),
        }

        i += 1;
    }
}
