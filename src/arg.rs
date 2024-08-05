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
}

impl fmt::Display for ARG {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

lazy_static! {
    static ref ARGS: HashMap<&'static str, ARG> = {
        let mut flags = HashMap::new();
        flags.insert(ARG::API.as_str(), ARG::API);
        flags.insert(ARG::CONFIG.as_str(), ARG::CONFIG);
        flags.insert(ARG::DEBUG.as_str(), ARG::DEBUG);
        flags.insert(ARG::DIRECTORY.as_str(), ARG::DIRECTORY);
        flags.insert(ARG::ENVIRONMENT.as_str(), ARG::ENVIRONMENT);
        flags.insert(ARG::EXECUTE.as_str(), ARG::EXECUTE);
        flags.insert(ARG::FILES.as_str(), ARG::FILES);
        flags.insert(ARG::HELP.as_str(), ARG::HELP);
        flags.insert(ARG::PRINT.as_str(), ARG::PRINT);
        flags.insert(ARG::PROJECT.as_str(), ARG::PROJECT);
        flags.insert(ARG::REQUIRED.as_str(), ARG::REQUIRED);
        flags.insert(ARG::SAVE.as_str(), ARG::SAVE);
        flags.insert(ARG::VERSION.as_str(), ARG::VERSION);
        flags.insert(ARG::UNKNOWN.as_str(), ARG::UNKNOWN);
        return flags;
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
            Some(ARG::UNKNOWN) | None => println!("UNKNOWN {}", flag),
        }

        i += 1;
    }
}
