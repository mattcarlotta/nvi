use lazy_static::lazy_static;
use std::collections::HashMap;
use std::fmt::{Display, Formatter, Result as FmtResult};

pub enum ARG {
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

impl ARG {
    pub fn as_str(&self) -> &'static str {
        match self {
            ARG::API => "--api",
            ARG::CONFIG => "--config",
            ARG::DEBUG => "--debug",
            ARG::DIRECTORY => "--directory",
            ARG::ENVIRONMENT => "--environment",
            ARG::EXECUTE => "--",
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
    pub static ref ARGS: HashMap<&'static str, ARG> = {
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
