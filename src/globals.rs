use crate::arg::Arg;
use std::collections::HashMap;
use std::env;
use std::path::PathBuf;
use std::sync::OnceLock;

// pub const SPACE: char = ' '; // 0x20
// pub const OPEN_BRACKET: char = '['; // 0x5b
// pub const CLOSE_BRACKET: char = ']'; // 0x5d
// pub const DOUBLE_QUOTE: char = '\"'; // 0x22
// pub const COMMA: char = ','; // 0x2c
pub const LINE_DELIMITER: char = '\n'; // 0x0a
pub const ASSIGN_OP: char = '='; // 0x3d
pub const HASH: char = '#'; // 0x23
pub const BACK_SLASH: char = '\\'; // 0x5c
pub const DOLLAR_SIGN: char = '$'; // 0x24
pub const OPEN_BRACE: char = '{'; // 0x7b
pub const CLOSE_BRACE: char = '}'; // 0x7d
pub const NULL_CHAR: char = '\0'; // 0x0

#[cfg(debug_assertions)]
pub const API_URL: &str = "http://127.0.0.1:4000";

#[cfg(not(debug_assertions))]
pub const API_URL: &str = "http://nvi.sh/api";

pub fn current_dir() -> &'static PathBuf {
    static CURRENT_DIR: OnceLock<PathBuf> = OnceLock::new();
    CURRENT_DIR.get_or_init(|| env::current_dir().expect("Failed to retrieve current directory."))
}

pub fn args() -> &'static HashMap<&'static str, Arg> {
    static HASHMAP: OnceLock<HashMap<&'static str, Arg>> = OnceLock::new();
    HASHMAP.get_or_init(|| {
        HashMap::from([
            (Arg::Api.as_str(), Arg::Api),
            (Arg::Config.as_str(), Arg::Config),
            (Arg::Debug.as_str(), Arg::Debug),
            (Arg::Directory.as_str(), Arg::Directory),
            (Arg::Environment.as_str(), Arg::Environment),
            (Arg::Execute.as_str(), Arg::Execute),
            (Arg::Files.as_str(), Arg::Files),
            (Arg::Help.as_str(), Arg::Help),
            (Arg::Print.as_str(), Arg::Print),
            (Arg::Project.as_str(), Arg::Project),
            (Arg::Required.as_str(), Arg::Required),
            (Arg::Save.as_str(), Arg::Save),
            (Arg::Version.as_str(), Arg::Version),
            (Arg::Unknown.as_str(), Arg::Unknown),
        ])
    })
}
