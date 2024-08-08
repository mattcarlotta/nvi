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
