use std::fmt::{Display, Formatter, Result as FmtResult};

pub enum Arg {
    Api,
    Config,
    Debug,
    Directory,
    Environment,
    Execute,
    Files,
    Help,
    Print,
    Project,
    Required,
    Save,
    Version,
    Unknown,
}

impl Arg {
    pub fn as_str(&self) -> &'static str {
        match self {
            Arg::Api => "--api",
            Arg::Config => "--config",
            Arg::Debug => "--debug",
            Arg::Directory => "--directory",
            Arg::Environment => "--environment",
            Arg::Execute => "--",
            Arg::Files => "--files",
            Arg::Help => "--help",
            Arg::Print => "--print",
            Arg::Project => "--project",
            Arg::Required => "--required",
            Arg::Save => "--save",
            Arg::Version => "--version",
            Arg::Unknown => "",
        }
    }
}

impl Display for Arg {
    fn fmt(&self, f: &mut Formatter) -> FmtResult {
        write!(f, "{}", self.as_str())
    }
}
